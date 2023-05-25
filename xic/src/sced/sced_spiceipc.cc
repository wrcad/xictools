
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

#ifdef DEMO_EXPORT

// If DEMO_EXPORT is defined, this file is being used in the
// stand-alone spclient WRspice IPC demo.  The code is pretty much
// identical to that used in Xic.  This could be simplified for the
// demo application, but I decided to leave it alone since it
// illustrates how things may be done, and it works.

#include "spclient.h"
#include "errorrec.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "pathlist.h"
#include "tvals.h"
#include "services.h"
#include "childproc.h"

#else

#include "main.h"
#include "fio.h"
#include "sced.h"
#include "sced_spmap.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "promptline.h"
#include "errorlog.h"

#include "miscutil/pathlist.h"
#include "miscutil/tvals.h"
#include "miscutil/services.h"
#include "miscutil/childproc.h"

#endif

#include "sced_spiceipc.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <errno.h>
#include <signal.h>
#ifdef WIN32
#include <io.h>
#include <winsock2.h>
#include <process.h>
#include <conio.h>
#include <fcntl.h>
#include "miscutil/msw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#endif

#ifdef __linux
#define sig_t sighandler_t
#endif

namespace {
    bool netdbg()
    {
        static bool checked;
        static bool set;

        if (!checked) {
            checked = true;
            set = (getenv("XTNETDEBUG") != 0);
        }
        return (set);
    }

    // Error message header.
    const char *SpiceIPC = "spice ipc";
}


//-----------------------------------------------------------------------
// WRspice Interface Functions
//-----------------------------------------------------------------------

#ifdef WIN32
#define CLOSESOCKET(x) shutdown(x, SD_SEND), closesocket(x)
#else
#define CLOSESOCKET(x) close(x)
#endif


// Fork (if local) and start program.  Establish connection to the
// process.
//
// Errs system will have message(s) on failure.
//
bool
cSpiceIPC::InitSpice()
{
    if (ipc_level > 0)
        CloseSpice();

    ipc_level = 1;
#ifdef SIGPIPE
    ipc_sigpipe_back = signal(SIGPIPE, SIG_IGN);
#endif
    char xic_host[128];
    gethostname(xic_host, 128);
    ipc_startup = true;
    if (ipc_parent_sp_port > 0) {
        // The application is being opened under the control of the
        // simulator.
        ipc_spice_port = ipc_parent_sp_port;
        hostent *hent = gethostbyname(xic_host);
        if (!hent) {
            Errs()->sys_herror("InitSpice: gethostbyname");
            return (false);
        }
        ipc_msg_skt = open_skt(hent, ipc_spice_port);
    }
    else {
        // The simulator is being opened under control of the application. 
        // Call back to the application to set up the variables.  Returned
        // strings are copied here.

        const char *spice_path, *spice_host, *host_prog, *display_name;
        if (!callback(&spice_path, &spice_host, &host_prog, &ipc_no_graphics,
                &ipc_no_toolbar, &display_name)) {
            Errs()->add_error(
                "InitSpice: Application callback triggered abort.");
            return (false);
        }
        delete [] ipc_spice_path;
        ipc_spice_path = lstring::copy(spice_path);
        delete [] ipc_spice_host;
        ipc_spice_host = lstring::copy(spice_host);
        delete [] ipc_host_prog;
        ipc_host_prog = lstring::copy(host_prog);
        delete [] ipc_display_name;
        ipc_display_name = lstring::copy(display_name);

        if (ipc_spice_host && *ipc_spice_host)
            ipc_msg_skt = init_remote(ipc_spice_host);
        else if (ipc_spice_path && *ipc_spice_path)
            ipc_msg_skt = init_local();
        else {
            Errs()->add_error(
                "InitSpice: No path to WRspice or remote host specified.");
            return (false);
        }
    }

    if (ipc_msg_skt > 0) {
#ifndef WIN32
        setup_async_io(true);
#endif

        // Send over the Xic window id.
        char tbuf[128];
        strcpy(tbuf, "winid ");
        if (DSPpkg::self()->GetMainWinIdentifier(tbuf + strlen(tbuf))) {
            char *tbf = send_to_spice(tbuf, 120);
            delete [] tbf;
        }
    }
    return (ipc_msg_skt > 0);
}


#ifdef DEMO_EXPORT
#else

namespace {
    // Allowed first token in analysis string.
    const char *anal_names[] = {
        "ac",
        "check",
        "dc",
        "disto",
        "loop",
        "noise",
        "op",
        "pz",
        "run",
        "send",  // send over deck, but don't run an analysis
        "sens",
        "tf",
        "tran",
        0
    };

    bool tok_valid(const char *tok)
    {
        if (tok) {
            for (const char **s = anal_names; *s; s++) {
                if (!strcmp(tok, *s))
                    return (true);
            }
        }
        return (false);
    }

    // Provision to recall the last few unique analysis strings, these
    // are cycled through with the arrow keys during prompting.

#define NUM_SAVED_ANAL 5
    hyList *saved_analyses[5];
    int saved_analysis_ptr;

    void save_analysis(hyList *hp)
    {
        if (!hp)
            return;
        for (int i = 0; i < NUM_SAVED_ANAL; i++) {
            if (!saved_analyses[i])
                break;
            if (!hyList::hy_strcmp(hp, saved_analyses[i])) {
                hyList::destroy(hp);
                return;
            }
        }
        hyList::destroy(saved_analyses[NUM_SAVED_ANAL - 1]);
        for (int i = NUM_SAVED_ANAL - 1; i > 0; i--)
            saved_analyses[i] = saved_analyses[i-1];
        saved_analyses[0] = hp;
    }

    void up_callback()
    {
        saved_analysis_ptr++;
        if (saved_analysis_ptr == NUM_SAVED_ANAL ||
                !saved_analyses[saved_analysis_ptr]) {
            saved_analysis_ptr--;
            return;
        }

        if (saved_analysis_ptr == -1)
            PL()->EditHypertextPrompt("Enter analysis command: ",
                SCD()->getAnalysisList(), false, PLedUpdate);
        else
            PL()->EditHypertextPrompt("Enter analysis command: ",
                saved_analyses[saved_analysis_ptr], false, PLedUpdate);
    }

    void down_callback()
    {
        saved_analysis_ptr--;
        if (saved_analysis_ptr < -1) {
            saved_analysis_ptr++;
            return;
        }

        if (saved_analysis_ptr == -1)
            PL()->EditHypertextPrompt("Enter analysis command: ",
                SCD()->getAnalysisList(), false, PLedUpdate);
        else
            PL()->EditHypertextPrompt("Enter analysis command: ",
                saved_analyses[saved_analysis_ptr], false, PLedUpdate);
    }
}

#endif


// Return true if job successfully launched or is in progress.
//
// The Run method, which is not used in the spclient demo, is the main
// initiator in Xic.  In Xic, an asynchronous interface is used.  Once
// the simulation starts, one may continue using Xic, and a message
// pops up when the simulation is done.  This is an important
// characteristic, but is not used in the demo.  The code can be
// studied to see how this is done.
//
bool
cSpiceIPC::RunSpice(CmdDesc *cmd)
{
#ifdef DEMO_EXPORT
    (void)cmd;
    return (false);
#else
    if (!DSP()->CurCellName()) {
        PL()->ShowPrompt("No current cell!");
        return (false);
    }
    CDs *cursde = CurCell(Electrical);
    if (!cursde || cursde->isEmpty()) {
        PL()->ShowPrompt("No electrical data found in current cell.");
        return (false);
    }
    if (ipc_in_spice && ipc_msg_skt >= 0) {
        PL()->ShowPrompt("WRspice analysis in progress.");
        if (cmd)
            Menu()->Select(cmd->caller);
        SCD()->PopUpSim(SpBusy);
        return (true);
    }
    if (cursde->cellname() != DSP()->TopCellName()) {
        PL()->ShowPrompt("Pop to top level first.");
        return (false);
    }
    SCD()->PopUpSim(SpNil);

    if (ipc_msg_skt < 0 ) {
        if (ipc_last_cir) {
            delete [] ipc_last_cir;
            ipc_last_cir = 0;
        }
        if (ipc_last_plot) {
            delete [] ipc_last_plot;
            ipc_last_plot = 0;
        }
        ipc_in_spice = false;
        InitSpice();
    }
    if (ipc_msg_skt < 0) {
        PL()->ShowPrompt("No connection to simulator.");
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        return (false);
    }

    // If the user pressed the "Run" button in the WRspice toolbar, a
    // run may have resumed, and this interface doesn't know it.  In
    // that case, the ipc calls will hang until the run is done or
    // timeout.  To avoid this, send over an interrupt to halt any run
    // in progress.
    // Don't send an interrupt if WRspice was just started, this can
    // cause WRspice to terminate.
    if (ipc_startup)
        ipc_startup = false;
    else
        InterruptSpice();

    char tbuf[64];
    const char *msg = "Connection broken.";

    // See the note in WRspice cp/ipcomm.cc about this interface.  For
    // WRspice 3.2.15 and later, the subcircuit catchar and name
    // mapping mode can be specified here.  However, we need to be
    // compatible with any release of WRspice.  I.e.,
    //   Pre-3.2.5, subc catchar fixed as ':', SPICE3 mode, ping
    //     returns "ok".
    //   3.2.5 and later, subc catchar set in WRspice, uses SPICE3
    //     mode, ping returns "ok<catchar>".
    //   3.2.15 and later, use this interface, i.e., catchar and
    //     mode can be set by Xic, ping returns "ok<catchar><mode>" if
    //     mode arg is given, "ok<catchar>" if not.
    //
    // The argument(s) to ping have the following syntax:
    //   [-sc<catchar>][ ][-sm<mode>] 
    // where <mode> is a digit character ('0' plus the enum value).

    snprintf(tbuf, sizeof(tbuf), "ping -sc%c-sm%c", CD()->GetSubcCatchar(),
        '0' + CD()->GetSubcCatmode());
    char *tbf = send_to_spice(tbuf, 10);
    if (!tbf) {
        PL()->ShowPrompt(msg);
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        return (false);
    }
    if (tbf[0] == 'o' && tbf[1] == 'k') {
        if (!tbf[2]) {
            // WRspice pre-3.2.5
            CDvdb()->setVariable(VA_SpiceSubcCatchar, ":");
            CDvdb()->setVariable(VA_SpiceSubcCatmode, "spice3");
        }
        else if (!tbf[3]) {
            // WRspice 3.2.5 - 3.2.14
            char bf[4];
            bf[0] = tbf[2];
            bf[1] = 0;
            CDvdb()->setVariable(VA_SpiceSubcCatchar, bf);
            CDvdb()->setVariable(VA_SpiceSubcCatmode, "spice3");
        }
        else {
            // WRspice 3.2.15 or later
            char bf[4];
            bf[0] = tbf[2];
            bf[1] = 0;
            CDvdb()->setVariable(VA_SpiceSubcCatchar, bf);
            if (tbf[3] == '0')
                CDvdb()->setVariable(VA_SpiceSubcCatmode, "wrspice");
            else
                CDvdb()->setVariable(VA_SpiceSubcCatmode, "spice3");
        }
    }
    else {
        // "can't happen"
        PL()->ErasePrompt();
        Log()->ErrorLog(SpiceIPC, "Protocol error: ping failed.");
        delete [] tbf;
        return (false);
    }
    delete [] tbf;

    if (inprogress()) {
        char *in = PL()->EditPrompt("Resume run in progress? ", "y");
        in = lstring::strip_space(in);
        if (!in) {
            PL()->ErasePrompt();
            return (false);
        }
        if (*in == 'y' || *in == 'Y') {
            if (SCD()->iplotStatusChanged()) {
                ClearIplot(true);
                if (ipc_msg_skt < 0) {
                    PL()->ShowPrompt(msg);
                    return (false);
                }
                SetIplot(true);
                if (ipc_msg_skt < 0) {
                    PL()->ShowPrompt(msg);
                    return (false);
                }
                SCD()->setIplotStatusChanged(false);
            }
            if (!runnit("resume")) {
                PL()->ShowPrompt(msg);
                if (Errs()->has_error())
                    Log()->ErrorLog(SpiceIPC, Errs()->get_error());
                return (false);
            }
            PL()->ErasePrompt();
            return (true);
        }
    }
    if (ipc_msg_skt < 0) {
        PL()->ShowPrompt(msg);
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        return (false);
    }

    saved_analysis_ptr = -1;
    PL()->RegisterArrowKeyCallbacks(down_callback, up_callback);
    hyList *anl = PL()->EditHypertextPrompt("Enter analysis command: ",
        SCD()->getAnalysisList(), false);
    PL()->RegisterArrowKeyCallbacks(0, 0);
    if (anl) {
        char *analysis_str = hyList::string(anl, HYcvPlain, false);
        const char *s = analysis_str;
        while (isspace(*s) || *s == '.')
            s++;
        const char *t = s;
        char *tok = lstring::gettok(&t);
        if (tok_valid(tok)) {
            delete [] tok;
            delete [] ipc_analysis;
            ipc_analysis = lstring::copy(s);
            delete [] analysis_str;
            save_analysis(SCD()->setAnalysisList(anl));
        }
        else if (tok) {
            delete [] tok;
            delete [] analysis_str;
            hyList::destroy(anl);
            PL()->ShowPromptV("Unknown analysis type %s.", tok);
            return (false);
        }
        else {
            delete [] tok;
            delete [] analysis_str;
            hyList::destroy(anl);
            PL()->ShowPrompt("An analysis command is required.");
            return (false);
        }
    }
    else {
        PL()->ErasePrompt();
        return (false);
    }

    DSPpkg::self()->SetWorking(true);
    stringlist *deck = SCD()->makeSpiceListing(cursde);
    if (!deck) {
        PL()->ShowPromptV(
            "Unknown error: no SPICE listing available for %s.",
            Tstring(cursde->cellname()));
        DSPpkg::self()->SetWorking(false);
        return (false);
    }
    if (!expand_includes(&deck, "decksource")) {
        PL()->ShowPrompt("Deck inclusion expansion failed.");
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        stringlist::destroy(deck);
        DSPpkg::self()->SetWorking(false);
        return (false);
    }
    char *outbuf;
    bool ok = deck_to_spice(deck, &outbuf);
    DSPpkg::self()->SetWorking(false);
    if (outbuf) {
        // Dump any stdout/stderr from WRspice to console.
        fprintf(stderr, "%s\n", outbuf);
        delete [] outbuf;
    }
    if (!ok && ipc_msg_skt > 0) {
        Errs()->get_error();  // Throw this away.
        PL()->ShowPrompt("WRspice returned error, source failed.");
        stringlist::destroy(deck);
        return (false);
    }
    stringlist::destroy(deck);
    if (ipc_msg_skt < 0) {
        PL()->ShowPrompt(msg);
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        return (false);
    }

    ClearIplot(true);
    if (ipc_msg_skt < 0) {
        PL()->ShowPrompt(msg);
        return (false);
    }
    SCD()->setIplotStatusChanged(false);

    // Delete the last circuit created here, if still around.
    if (ipc_last_cir && strcmp(ipc_last_cir, "none")) {
        snprintf(tbuf, sizeof(tbuf), "freecir %s", ipc_last_cir);
        tbf = send_to_spice(tbuf, 120);
        if (!tbf) {
            PL()->ShowPrompt(msg);
            if (Errs()->has_error())
                Log()->ErrorLog(SpiceIPC, Errs()->get_error());
            return (false);
        }
        delete [] tbf;
    }

    SetIplot(true);
    if (ipc_msg_skt < 0) {
        PL()->ShowPrompt(msg);
        return (false);
    }

    // Delete the last plot generated here, if still around.
    if (ipc_last_plot && strcmp(ipc_last_plot, "none")) {
        snprintf(tbuf, sizeof(tbuf), "freeplot %s", ipc_last_plot);
        tbf = send_to_spice(tbuf, 120);
        if (!tbf) {
            PL()->ShowPrompt(msg);
            if (Errs()->has_error())
                Log()->ErrorLog(SpiceIPC, Errs()->get_error());
            return (false);
        }
        delete [] tbf;
    }
    if (!strcmp(ipc_analysis, "send")) {
        PL()->ShowPrompt("Deck sent to WRspice, is the current circuit.");
        return (true);
    }

    if (!runnit(ipc_analysis)) {
        PL()->ShowPrompt(msg);
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        return (false);
    }
    PL()->ErasePrompt();
    return (true);
#endif
}


// Send a command to WRspice for execution.  If the command is sent to
// WRspice, true is returned.  The retbuf will contain the return
// message for the command, which may contain a message, but is
// usually just the protocol return token "ok".  If false is returned,
// i.e., the simulator is inaccessible, errbuf will contain a message. 
// The outbuf will contain the unified stdio/stderr produced by the
// command, if any.  The errbuf will contain an error message if there
// is an error.  Any of these pointers can be null, in which case the
// corresponding output is discarded.  Returned strings are owned by
// the caller and should be freed.
//
// If databuf is not null, certain interface commands that return
// binary data will create a returned data block.  The returned data
// is owned by the caller and should be freed.
//
bool
cSpiceIPC::DoCmd(const char *cmd, char **retbuf, char **outbuf,
    char **errbuf, unsigned char **databuf)
{
    if (retbuf)
        *retbuf = 0;
    if (outbuf)
        *outbuf = 0;
    if (errbuf)
        *errbuf = 0;
    if (databuf)
        *databuf = 0;

    if (ipc_in_spice && ipc_msg_skt >= 0) {
        if (errbuf)
            *errbuf = lstring::copy("WRspice analysis in progress.");
        SCD()->PopUpSim(SpBusy);
        Log()->WarningLog(SpiceIPC, 
            "WRspice is busy, can't run command unless\n"
            "analysis is paused or finished.");
        return (false);
    }

    if (ipc_msg_skt < 0) {
        ipc_in_spice = false;
        InitSpice();
    }
    if (ipc_msg_skt < 0) {
        if (retbuf)
            *retbuf = lstring::copy("No connection to simulator.");
        if (Errs()->has_error()) {
            if (errbuf)
                *errbuf = lstring::copy(Errs()->get_error());
            else
                Errs()->get_error();  // Clear error msg buffer.
        }
        return (false);
    }

    // Don't send an interrupt if WRspice was just started, this can
    // cause WRspice to terminate.
    if (ipc_startup)
        ipc_startup = false;
    else if (!ipc_no_toolbar)
        InterruptSpice();  // In case run restarted from WRspice toolbar.

    // Special command: send filename
    // This will send the local file to WRspice for sourcing.
    if (lstring::cimatch("send", cmd)) {
        const char *s = cmd;
        lstring::advtok(&s);
        if (!FileToSpice(s, outbuf)) {
            if (retbuf)
                *retbuf = lstring::copy("File transfer/source failed.");
            if (Errs()->has_error()) {
                if (errbuf)
                    *errbuf = lstring::copy(Errs()->get_error());
                else
                    Errs()->get_error();  // Clear error msg buffer.
            }
        }
        else if (retbuf)
            *retbuf = lstring::copy("File transfer succeded.");
    }
    else if (cmd && *cmd) {
        // Switch back to synchronous stdio/stderr for this
        // transaction, since we read this return explicitly.  If
        // set_async fails (highly unlikely) warn but ignore it.
        //
        if (!set_async(ipc_stdout_skt, false))
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());

        // We don't use send_to_spice here (see comment in
        // send_to_spice function) since we need to read stdout
        // before reading the message return to avoid hanging the
        // program.  WRspice stops writing stdout when the internal
        // buffer becomes full, so we need to read stdout before
        // reading the message return.

        if (!write_msg(cmd, ipc_msg_skt)) {
            if (retbuf)
                *retbuf = lstring::copy("Connection broken, write failed.");
            if (Errs()->has_error()) {
                if (errbuf)
                    *errbuf = lstring::copy(Errs()->get_error());
                else
                    Errs()->get_error();  // Clear error msg buffer.
            }
            if (!set_async(ipc_stdout_skt, true))
                Log()->ErrorLog(SpiceIPC, Errs()->get_error());
            return (false);
        }

        Errs()->init_error();
        bool ok = read_cmd_return(retbuf, outbuf, databuf);
        if (Errs()->has_error()) {
            if (errbuf)
                *errbuf = lstring::copy(Errs()->get_error());
            else
                Errs()->get_error();  // Clear error msg buffer.
        }
        if (!ok)
            return (false);

        if (!set_async(ipc_stdout_skt, true))
            Log()->WarningLog(SpiceIPC, Errs()->get_error());
    }
    return (true);
}


// Send the local file to WRspice.  The file will have .inc/.lib lines
// expanded locally.  WRspice will source the file.  Any stdout/stderr
// is returned in outbuf.
//
// On error, return false, with message(s) in the Errs system.
//
bool
cSpiceIPC::FileToSpice(const char *fname, char **outbuf)
{
    char *ptmp = pathlist::expand_path(fname, false, false);
    FILE *fp = fopen(ptmp, "r");
    delete [] ptmp;
    if (!fp) {
        Errs()->sys_error("FileToSpice: fopen");
        Errs()->add_error("FileToSpice: Couldn't open file %s.", fname);
        return (false);
    }

    stringlist *s0 = 0, *se = 0;
    bool err = false;
    for (;;) {
        // Let WRspice handle HSPICE comment forms.
        char *buf = ReadLine(fp, false);
        if (!buf)
            break;
        if (!s0)
            s0 = se = new stringlist(buf, 0);
        else {
            se->next = new stringlist(buf, 0);
            se = se->next;
        }
    }
    fclose(fp);

    if (err) {
        stringlist::destroy(s0);
        return (false);
    }
#ifdef DEMO_EXPORT
    // In the demo, we don't expand the .include/.lib and similar lines
    // locally.
#else
    if (!expand_includes(&s0, "decksource")) {
        stringlist::destroy(s0);
        return (false);
    }
#endif
    bool ok = deck_to_spice(s0, outbuf);
    stringlist::destroy(s0);
    return (ok);
}


// Plot some data using the WRspice plot package.
//
bool
cSpiceIPC::ExecPlot(const char *str)
{
    if (ipc_in_spice) {
        PL()->ShowPrompt("WRspice analysis in progress, please wait.");
        return (false);
    }
    if (ipc_msg_skt < 0) {
        PL()->ShowPrompt("No active connection to WRspice.");
        return (false);
    }

    // Don't send an interrupt if WRspice was just started, this can
    // cause WRspice to terminate.
    if (ipc_startup)
        ipc_startup = false;
    else if (!ipc_no_toolbar)
        InterruptSpice();  // In case run restarted from WRspice toolbar.

    // Switch back to synchronous stdio/stderr for this
    // transaction, since we read this return explicitly.  If
    // set_async fails (highly unlikely) warn but ignore it.
    //
    if (!set_async(ipc_stdout_skt, false))
        Log()->WarningLog(SpiceIPC, Errs()->get_error());

    // We don't use send_to_spice here (see comment in
    // send_to_spice function) since we need to read stdout
    // before reading the message return to avoid hanging the
    // program.  WRspice stops writing stdout when the internal
    // buffer becomes full, so we need to read stdout before
    // reading the message return.

    sLstr lstr;
    lstr.add("plot ");
    lstr.add(str);

    Errs()->init_error();
    if (!write_msg(lstr.string(), ipc_msg_skt)) {
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        if (!set_async(ipc_stdout_skt, true))
            Log()->WarningLog(SpiceIPC, Errs()->get_error());
        return (false);
    }

    char *outbuf;
    bool ok = read_cmd_return(0, &outbuf, 0);
    if (Errs()->has_error())
        Log()->ErrorLog(SpiceIPC, Errs()->get_error());
    else if (outbuf) {
        // Some error messages in the plot command go t stderr.
        Log()->ErrorLog(SpiceIPC, outbuf);
        delete [] outbuf;
    }
    if (!ok)
        return (false);

    if (!set_async(ipc_stdout_skt, true))
        Log()->WarningLog(SpiceIPC, Errs()->get_error());

    return (true);
}


// Set up an iplot in WRspice.
//
bool
cSpiceIPC::SetIplot(bool no_intr)
{
    if (!SCD()->doingIplot())
        return (false);
    char *s = SCD()->getIplotCmd(false);
    if (s) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "iplot %s", s);
        delete [] s;
        if (ipc_in_spice) {
            PL()->ShowPrompt("WRspice analysis in progress, please wait.");
            return (false);
        }
        if (ipc_msg_skt < 0) {
            PL()->ShowPrompt("No active connection to WRspice.");
            return (false);
        }
        if (!no_intr)
            InterruptSpice();  // In case run restarted from WRspice toolbar.
        char *tbf = send_to_spice(buf, 120);
        if (!tbf) {
            if (Errs()->has_error())
                Log()->ErrorLog(SpiceIPC, Errs()->get_error());
            return (false);
        }
        delete [] tbf;
    }
    return (true);
}


// Clear all iplots in WRspice.
//
bool
cSpiceIPC::ClearIplot(bool no_intr)
{
    if (ipc_in_spice) {
        PL()->ShowPrompt("WRspice analysis in progress, please wait.");
        return (false);
    }
    if (ipc_msg_skt < 0) {
        PL()->ShowPrompt("No active connection to WRspice.");
        return (false);
    }
    if (!no_intr)
        InterruptSpice();  // In case run restarted from WRspice toolbar.
    char *tbf = send_to_spice("delete iplot", 120);
    if (!tbf) {
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        return (false);
    }
    delete [] tbf;
    return (true);
}


// While running simulations, pass interrupt to simulator.
//
void
cSpiceIPC::InterruptSpice()
{
    if (!ipc_parent_sp_pid && !ipc_child_sp_pid && ipc_msg_skt > 0) {
        // Remote WRspice through wrspiced.
        char c = 0;
        send(ipc_msg_skt, &c, 1, MSG_OOB);
        return;
    }
    int pid = 0;
    if (ipc_parent_sp_pid)
        pid = ipc_parent_sp_pid;    // Xic is child of WRspice.
    else if (ipc_child_sp_pid)
        pid = ipc_child_sp_pid;     // Local WRspice.
    else
        return;

#ifdef HAVE_KILL
    kill(pid, SIGINT);
#else
#ifdef WIN32
    char buf[64];
    snprintf(buf, sizeof(buf),"wrspice.sigint.%d", pid);
    HANDLE hs = OpenSemaphore(SEMAPHORE_MODIFY_STATE, false, buf);
    if (hs) {
        long pv;
        ReleaseSemaphore(hs, 1, &pv);
        CloseHandle(hs);
    }
#endif
#endif
}


// Tell WRspice about program termination.
//
void
cSpiceIPC::CloseSpice()
{
    // Halt run, if any.
    InterruptSpice();
    if (ipc_msg_skt > 0) {
        char buf[64];
        if (ipc_parent_sp_pid) {
            // Tell WRspice parent that we are about to terminate.
            snprintf(buf, sizeof(buf), "close %d", (int)getpid());
            write_msg(buf, ipc_msg_skt);
        }
        else if (ipc_child_sp_pid) {
            // Terminate WRspice child.
#ifndef WIN32
            Proc()->RemoveChildHandler(ipc_child_sp_pid, child_hdlr);
#endif
            write_msg("close 0", ipc_msg_skt);
            Menu()->MenuButtonSet(MMmain, MenuRUN, false);
        }
    }
    close_all();
}


// This checks if there really is something to read.  In UNIX,
// this is called when something is in queue to read.  In Win32,
// use select to wait until the socket is ready for reading,
// called from another thread.
//
void
cSpiceIPC::SigIOhdlr(int sig)
{
    if (ipc_msg_skt < 0)
        return;
    bool err = false;
    fd_set readfds;
    for (;;) {
        FD_ZERO(&readfds);
        FD_SET(ipc_msg_skt, &readfds);
        int tfd = ipc_msg_skt;
        timeval to;
#ifdef WIN32
        (void)sig;
        to.tv_sec = 5;
        to.tv_usec = 0;
        int i = select(tfd+1, &readfds, 0, 0, &to);
        if (i == 0)
            continue;
#else
        to.tv_sec = 0;
        to.tv_usec = 0;
        if (ipc_stdout_skt > 0) {
            FD_SET(ipc_stdout_skt, &readfds);
            if (ipc_stdout_skt > tfd)
                tfd = ipc_stdout_skt;
        }
        int i = select(tfd+1, &readfds, 0, 0, &to);
        if (i == 0) {
            // not ready, not our fd so should be called again
            if (ipc_sigio_back && ipc_sigio_back != SIG_IGN &&
                    ipc_sigio_back != SIG_DFL)
                (*ipc_sigio_back)(sig);
            return;
        }
#endif
        else if (i < 0) {
            if (errno == EINTR)
                continue;
            err = true;
        }
        break;
    }

#ifdef WIN32
    setup_async_io(false);
#endif
    if (err) {
        // WRspice must have died.
        ipc_in_spice = false;
        set_async(ipc_msg_skt, false);       // May add error message.
        signal(SIGINT, ipc_sigint_back);
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        SCD()->PopUpSim(SpError);
        Menu()->MenuButtonSet(MMmain, MenuRUN, false);
        return;
    }
    if (FD_ISSET(ipc_msg_skt, &readfds)) {
        // WRspice run finished or paused.
        ipc_in_spice = false;
        set_async(ipc_msg_skt, false);       // May add error message.
        signal(SIGINT, ipc_sigint_back);
        bool ok = complete_spice();
        if (Errs()->has_error())
            Log()->ErrorLog(SpiceIPC, Errs()->get_error());
        if (!ok) {
            SCD()->PopUpSim(SpError);
            Menu()->MenuButtonSet(MMmain, MenuRUN, false);
        }
    }
    if (ipc_stdout_skt > 0 && FD_ISSET(ipc_stdout_skt, &readfds)) {
        // WRspice put something into stdout/stderr, dump it to console.
        if (!err)
            dump_stdout();
    }
}


// Static function.
// Read a line of input, including the trailing newline.  Do some
// comment recognition.  Return null if fp is at EOF.
//
char *
cSpiceIPC::ReadLine(FILE *fp, bool hs_compat)
{
    sLstr lstr;
    int c;
    while ((c = getc(fp)) != EOF) {
        if (c == '\r')
            continue;  // File may come from MS Windows.
        lstr.add_c(c);
        if (c == '\n')
            break;
    }
    if (!lstr.length())
        return (0);

    // The hs_compat arg is for HSPICE compatibility.  Look for
    // '$' or ';' characters.  These may indicate comment text.

    char *line = lstr.string_trim();
    char *l = line;
    while (isspace(*l))
        l++;

    if (*l == '$' || *l ==';') {
        if (hs_compat || isspace(*(l+1)))
            *l = '*';
    }
    else if (*l) {
        bool insq = false, indq = false;
        for (char *s = l+1; *s; s++) {
            if (*s == '\'' && s[-1] != '\\') {
                insq = !insq;
                continue;
            }
            if (*s == '"' && s[-1] != '\\') {
                indq = !indq;
                continue;
            }
            if (insq || indq)
                continue;

            if ((*s == '$' || *s == ';') && s[-1] != '\\' &&
                    (isspace(s[-1]) || hs_compat)) {
                if (isspace(s[1]) || hs_compat) {
                    s[0] = '\n';
                    s[1] = 0;
                    break;
                }
            }
        }
    }
    return (line);
}


// Initialize/reinitialize all member values.
//
void
cSpiceIPC::ctor_init()
{
    ipc_analysis = 0;
    ipc_last_cir = 0;
    ipc_last_plot = 0;
    ipc_sigint_back = 0;
    ipc_sigio_back = 0;
    ipc_sigpipe_back = 0;
    ipc_msg_skt = -1;
    ipc_stdout_skt = -1;
    ipc_stdout_skt2 = -1;
#ifdef WIN32
    ipc_stdout_hrd = 0;
#endif
    ipc_spice_port = 0;
    ipc_child_sp_pid = 0;
    ipc_parent_sp_port = 0;
    ipc_parent_sp_pid = 0;

#ifdef WIN32
    ipc_stdout_async = false;
#endif
    ipc_in_spice = false;
    ipc_startup = false;
    ipc_level = 0;

    ipc_spice_path = 0;
    ipc_spice_host = 0;
    ipc_host_prog = 0;
    ipc_display_name = 0;
    ipc_no_graphics = false;
    ipc_no_toolbar = false;
    ipc_msg_pgid = 0;
    ipc_stdout_pgid = 0;
}


namespace {
    const char *bad_init_msg =
        "WRspice graphics initialization failed, permission problem?\n";

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


// Initialize remote connection, return i/o socket.  On error, return
// -1, with a message in the Errs system.
//
int
cSpiceIPC::init_remote(const char *c_spice_host)
{
    // Make a local copy, se we can null out the colon, if any.
    char spice_host[256];
    strcpy(spice_host, c_spice_host);

    char xic_host[128];
    gethostname(xic_host, 128);

    // X display on the local machine.
    char display_string[128];
    *display_string = 0;
    bool has_graphics = !ipc_no_graphics;
#ifdef DEMO_EXPORT
    // The demo app is command-line driven.
#else
    if (has_graphics && DSPpkg::self()->UsingX11()) {
        const char *ds = DSPpkg::self()->GetDisplayString();
        if (ds) {
            if (ipc_display_name)
                strcpy(display_string, ipc_display_name);
            else
                strcpy(display_string, ds);
        }
        else
            has_graphics = false;
    }
#endif

    // Look for :portnum in host name, if found set wrspiced_port
    // and strip from name.
    //
    unsigned short wrspiced_port = 0;
    char *col = strrchr(spice_host, ':');
    if (col && check_digits(col+1)) {
        wrspiced_port = atoi(col+1);
        *col = 0;
    }
    if (!*spice_host)
        strcpy(spice_host, "localhost");

    hostent *hent = gethostbyname(xic_host);
    if (!hent) {
        Errs()->sys_herror("init_remote: gethostbyname");
        return (-1);
    }

    hent = gethostbyname(spice_host);
    if (!hent) {
        Errs()->sys_herror("init_remote: gethostbyname");
        return (-1);
    }

    if (wrspiced_port <= 0) {
        servent *sp = getservbyname(WRSPICE_SERVICE, "tcp");
        if (sp)
            wrspiced_port = ntohs(sp->s_port);
        else
            wrspiced_port = WRSPICE_PORT;
    }
    protoent *pp = getprotobyname("tcp");
    if (pp == 0) {
        Errs()->add_error("init_remote: Unknown protocol tcp.\n");
        return (-1);
    }
    sockaddr_in skt;
    memset(&skt, 0, sizeof(sockaddr_in));
    memcpy(&skt.sin_addr, hent->h_addr, hent->h_length);
    skt.sin_family = hent->h_addrtype;
    skt.sin_port = htons(wrspiced_port);

    // Check screen access for remote host.  Skip this if we provided
    // the display name.
    if (has_graphics && !ipc_display_name &&
            !DSPpkg::self()->CheckScreenAccess(hent, spice_host,
            display_string)) {
        has_graphics = false;
#ifndef DEMO_EXPORT
        Log()->WarningLogV(SpiceIPC,
            "Remote host %s is not listed in the X server's access\n"
            "database.  You may need to restart %s after enabling access\n"
            "with \"xhost +%s\", or use ssh X11 forwarding, for WRspice\n"
            "graphics.\n", spice_host, XM()->Product(), spice_host);
#endif
    }

    // Create the socket.
    int sd = socket(AF_INET, SOCK_STREAM, pp->p_proto);
    if (sd < 0) {
        Errs()->sys_error("init_remote: socket");
        return (-1);
    }

    bool connected = (connect(sd, (sockaddr*)&skt, sizeof(sockaddr)) == 0);
    if (!connected && wrspiced_port == WRSPICE_PORT) {
        skt.sin_port = htons(3004);
        CLOSESOCKET(sd);
        sd = socket(AF_INET, SOCK_STREAM, pp->p_proto);
        if (sd < 0) {
            Errs()->sys_error("init_remote: socket");
            return (-1);
        }
        connected = (connect(sd, (sockaddr*)&skt, sizeof(sockaddr)) == 0);
        if (connected) {
            Log()->WarningLog(SpiceIPC,
                "WARNING: connected to wrspiced on old unregistered\n"
                "port number 3004.  Please update the WRspice package,\n"
                "which provides wrspiced that will use IANA registered\n"
                "port number 6114.");
            wrspiced_port = 3004;
        }
    }
    if (!connected) {
        Errs()->add_error("init_remote: Can't connect to wrspiced on %s:%d.",
            spice_host, wrspiced_port);
        CLOSESOCKET(sd);
        return (-1);
    }

    // Check the server protocol.
    bool use_old_protocol = false;
    if (!write_msg("version", sd))
        return (-1);
    char *tbf = 0;
    if (!read_msg(sd, 120, &tbf)) {
        Errs()->get_error();    // clear error message buffer.
        // The server is pre wrspice-3.2.9, and has closed the
        // connection since it doesn't recognize the "version"
        // identifier.  Reopen the socket.
        sd = socket(AF_INET, SOCK_STREAM, pp->p_proto);
        if (sd < 0) {
            Errs()->sys_error("init_remote: socket");
            return (-1);
        }
        if (connect(sd, (sockaddr*)&skt, sizeof(sockaddr)) < 0) {
            Errs()->sys_error("init_remote: connect");
            CLOSESOCKET(sd);
            return (-1);
        }
        use_old_protocol = true;
    }
    // Don't care about the return yet.
    delete [] tbf;

    const char *user = "anonymous";
#ifdef HAVE_PWD_H
    passwd *pw = getpwuid(getuid());
    if (pw)
        user = pw->pw_name;
#endif

    // Send initialization string.
    char buf[256];
    if (use_old_protocol) {
        if (has_graphics)
            snprintf(buf, sizeof(buf), "%s %s",
                ipc_no_toolbar ? "xic_user_nt" : "xic_user", display_string);
        else
            snprintf(buf, sizeof(buf), "%s %s", "xic_user_nt", "none");
    }
    else {
        if (has_graphics)
            snprintf(buf, sizeof(buf), "%s:%s %s", user,
                ipc_no_toolbar ? "-t" : "+t", display_string);
        else
            snprintf(buf, sizeof(buf), "%s:-t %s", user, xic_host);
    }
    if (ipc_host_prog && *ipc_host_prog) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, " %s", ipc_host_prog);
    }

    if (!write_msg(buf, sd))
        return (-1);

    tbf = 0;
    if (!read_msg(sd, 120, &tbf))
        return (-1);
    if (!strcmp(tbf, "toomany")) {
        Errs()->add_error("init_remote: Job refused by %s: load exceeded.\n"
            "Please use another host or try again later.\n", spice_host);
        CLOSESOCKET(sd);
        delete [] tbf;
        return (-1);
    }
    else if (strcmp(tbf, "ok")) {
        Errs()->add_error("init_remote: Protocol error from %s: %s.\n",
            spice_host, tbf);
        CLOSESOCKET(sd);
        delete [] tbf;
        return (-1);
    }
    delete [] tbf;

    // OK, we have a connection to stdio.  Now set up the connection
    // that we really use.  First grab the port.
    //
    int port = -1;
    bool gr_ok = true;
    stringlist *s0 = 0, *se = 0;

    // Here we read the WRspice banner to get the port number. 
    // There will be a port number line, unless wrspice dies (3.2.10
    // and earlier died if the display couldn't be opened).  Allow
    // for possible error message blather, 20 lines should be
    // enough.

    for (int cnt = 0; cnt < 20; cnt++) {
        tbf = 0;
        if (!read_msg(sd, 120, &tbf, true))
            break;
        if (!s0)
            s0 = se = new stringlist(tbf, 0);
        else {
            se->next = new stringlist(tbf, 0);
            se = se->next;
        }
        if (lstring::ciprefix("load", tbf)) {
            // Ignore lines starting with "Load", these include device
            // module loading, of which there can be arbitrarily many.

            cnt--;
            continue;
        }

        if (!strncmp(tbf, "Warning: no graphics", 20))
            gr_ok = false;
        else if (!strncmp(tbf, "port ", 5)) {
            lstring::advtok(&tbf);
            port = atoi(tbf);
            break;
        }
    }

    if (port < 0) {
        sLstr lstr;
        lstr.add("init_remote: No port from WRspice daemon on ");
        lstr.add(spice_host);
        lstr.add(".\nMessages returned:\n");
        for (stringlist *sl = s0; sl; sl = sl->next) {
            lstr.add(sl->string);
            lstr.add_c('\n');
        }
        Errs()->add_error(lstr.string());
        stringlist::destroy(s0);
        CLOSESOCKET(sd);
        return (-1);
    }
    stringlist::destroy(s0);

    int tfd = sd;
    sd = open_skt(hent, port);
    if (sd < 0)
        CLOSESOCKET(tfd);
    else {
        printf("Stream established to WRspice daemon on %s, port %d.\n",
            spice_host, port);
        if (has_graphics && !gr_ok)
            Log()->WarningLog(SpiceIPC, bad_init_msg);
        ipc_stdout_skt = tfd;
#ifdef WIN32
        _beginthread(remote_stdout_thread_proc, 0, this);
#endif
        // Put the stdout channel into async mode.  Things like the
        // Print button in the WRspice Vectors tool need to pass
        // output to the console asynchronously.
        //
        if (!set_async(ipc_stdout_skt, true))
            fprintf(stderr, "%s\n", Errs()->get_error());
    }
    return (sd);
}


// Initialize local connection, return i/o socket.  On error, return
// -1, with a message in the Errs system.
//
int
cSpiceIPC::init_local()
{
    if (!ipc_spice_path || !*ipc_spice_path) {
        Errs()->add_error("init_local: No path to WRspice given.");
        return (-1);
    }
    char xic_host[128];
    gethostname(xic_host, 128);

    const char *display_string = 0;
    bool has_graphics = !ipc_no_graphics;
#ifdef DEMO_EXPORT
    // The demo app is command-line driven.
#else
    if (has_graphics && DSPpkg::self()->UsingX11()) {
        display_string = DSPpkg::self()->GetDisplayString();
        if (!display_string)
            has_graphics = false;
    }
#endif

    const char *prog_name = lstring::strrdirsep(ipc_spice_path);
    if (!prog_name)
        prog_name = ipc_spice_path;
    else
        prog_name++;

#define MAX_LINES 50
#ifdef WIN32
    // Create a pipe for reading stdout.
    HANDLE stdout_hwr;
    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = true;
    if (!CreatePipe(&ipc_stdout_hrd, &stdout_hwr, &sa, 0)) {
        Errs()->add_error("init_local: CreatePipe system call failed.");
        return (-1);
    }

    // Prevent inheritance of read end.
    SetHandleInformation(ipc_stdout_hrd, HANDLE_FLAG_INHERIT, 0);

    // Dup the stderr handle, in case stderr or stdout is closed in child.
    HANDLE stderr_hwr;
    if (!DuplicateHandle(GetCurrentProcess(), stdout_hwr,
            GetCurrentProcess(), &stderr_hwr, 0, true,
            DUPLICATE_SAME_ACCESS)) {
        Errs()->add_error("init_local: DuplicateHandle system call failed.");
        return (-1);
    }

    char *cmd = new char[strlen(ipc_spice_path) + 16];
    strcpy(cmd, ipc_spice_path);
    strcat(cmd, " -P");
    if (has_graphics) {
        if (!ipc_no_toolbar)
            strcat(cmd, " -I");
    }
    else
        strcat(cmd, " -Dnone");

    PROCESS_INFORMATION *info = msw::NewProcess(0, cmd, DETACHED_PROCESS,
        true, 0, stdout_hwr, stderr_hwr);
    if (!info) {
        Errs()->add_error(
            "init_local: Failed to create new process.\ncmd: %s", cmd);
        delete [] cmd;
        return (-1);
    }
    CloseHandle(stdout_hwr);
    CloseHandle(stderr_hwr);
    WaitForInputIdle(info->hProcess, 5000);

    int port = -1;
    bool gr_ok = true;
    bool done = false;
    for (int i = 0; i < MAX_LINES; i++) {
        char buf[1024];
        int cnt = 0;
        for (int j = 0; j < 1024; j++) {
            DWORD nb;
            if (i) {
                // If wrspice dies, ReadFile hangs (no, shouldn't anymore).
                PeekNamedPipe(ipc_stdout_hrd, 0, 0, 0, &nb, 0);
                if (nb == 0) {
                    Sleep(1000);
                    PeekNamedPipe(ipc_stdout_hrd, 0, 0, 0, &nb, 0);
                    if (nb == 0) {
                        done = true;
                        break;
                    }
                }
            }
            if (!ReadFile(ipc_stdout_hrd, buf+cnt, 1, &nb, 0) || !nb) {
                done = true;
                break;
            }
            cnt++;
            if (buf[cnt-1] == '\n') {
                buf[cnt] = 0;
                if (!strncmp(buf, "Warning: no graphics", 20))
                    gr_ok = false;
                else if (!strncmp(buf, "port ", 5)) {
                    port = atoi(buf + 5);
                    done = true;
                }
                break;
            }
        }
        if (done)
            break;
    }
    delete [] cmd;
    pid_t pid = info->dwProcessId;

    if (port < 0 || !gr_ok) {
        terminate_spice(pid);
        Errs()->add_error(
            "init_local: Failed to initialize WRspice, terminated.");
        return (-1);
    }

    // The following mess basically implements socketpair.
    //
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        Errs()->sys_herror("init_local: socket");
        return (-1);
    }
    struct sockaddr_in skt;
    skt.sin_family = AF_INET;
    skt.sin_addr.s_addr = INADDR_ANY;
    skt.sin_port = 0;
    if (bind(sd, (struct sockaddr*)&skt, sizeof(skt))) {
        Errs()->sys_herror("init_local: bind");
        return (-1);
    }
    socklen_t len = sizeof(skt);
    if (getsockname(sd, (struct sockaddr*)&skt, &len)) {
        Errs()->sys_herror("init_local: getsockname");
        return (-1);
    }
    listen(sd, 1);
    int out_acct = sd;
    int out_port = ntohs(skt.sin_port);

    hostent *hent = gethostbyname(xic_host);
    if (!hent) {
        Errs()->sys_herror("init_local: gethostbyname");
        return (-1);
    }
    sd = open_skt(hent, out_port);
    ipc_stdout_skt2 = sd;

    int msg = accept(out_acct, 0, 0);
    if (msg < 0) {
        Errs()->sys_herror("init_local: accept");
        return (-1);
    }
    ipc_stdout_skt = msg;

    _beginthread(local_stdout_thread_proc, 0, this);
    _beginthread(child_thread_proc, 0, info);

#else
    // Use a socket pair instead of a pipe for stdout transfer, since
    // pipes don't support O_ASYNC in RHEL3.
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, &ipc_stdout_skt) < 0) {
        Errs()->sys_error("init_local: socketpair");
        return (-1);
    }

    // fork child on this cpu
    int pid = fork();
    if (pid == -1) {
        Errs()->sys_error("init_local: fork");
        return (-1);
    }
    if (!pid) {
        DSPpkg::self()->CloseGraphicsConnection();
        dup2(ipc_stdout_skt2, fileno(stdout));
        dup2(ipc_stdout_skt2, fileno(stderr));
        if (has_graphics && *display_string) {
            if (ipc_no_toolbar)
                execl(ipc_spice_path, prog_name, "-P", "-D",
                    display_string, (char*)0);
            else
                execl(ipc_spice_path, prog_name, "-P", "-I", "-D",
                    display_string, (char*)0);
        }
        else
            execl(ipc_spice_path, prog_name, "-P", "-Dnone", (char*)0);
        Errs()->sys_error("init_local: execl");
        _exit(1);
    }
    Proc()->RegisterChildHandler(pid, child_hdlr, 0);

    int port = -1;
    bool gr_ok = true;
    bool done = false;
    for (int i = 0; i < MAX_LINES; i++) {
        char buf[1024];
        int cnt = 0;
        while (cnt < 1023) {
            int k = read(ipc_stdout_skt, buf+cnt, 1);
            if (k == 0) {
                // End of data, something's wrong.
                done = true;
                break;
            }
            if (k < 0) {
                if (errno == EINTR)
                    continue;
                Errs()->sys_error("init_local: read");
                done = true;
                break;
            }
            if (k) {
                cnt++;
                if (buf[cnt-1] == '\n') {
                    buf[cnt] = 0;
                    if (!strncmp(buf, "Warning: no graphics", 20))
                        gr_ok = false;
                    else if (!strncmp(buf, "port ", 5)) {
                        port = atoi(buf + 5);
                        done = true;
                    }
                    break;
                }
            }
        }
        if (done)
            break;
    }
    if (port < 0) {
        terminate_spice(pid);
        Errs()->add_error(
            "init_local: Failed to initialize WRspice, terminated.");
        return (-1);
    }

    hostent *hent = gethostbyname(xic_host);
    if (!hent) {
        Errs()->sys_herror("init_local: gethostbyname");
        return (-1);
    }
#endif

    // Put the stdout channel into async mode.  Things like the
    // Print button in the WRspice Vectors tool need to pass
    // output to the console asynchronously.
    //
    if (!set_async(ipc_stdout_skt, true))
        fprintf(stderr, "%s\n", Errs()->get_error());

    int msd = open_skt(hent, port);
    if (msd < 0) {
        terminate_spice(pid);
        Errs()->add_error("init_local: Failed to open socket.");
        return (-1);
    }
    ipc_child_sp_pid = pid;
    ipc_spice_port = port;
    printf("Simulator: %s\n", ipc_spice_path);
    printf("Stream established to simulator, port %d.\n", port);
    if (has_graphics && !gr_ok)
        Log()->WarningLog(SpiceIPC, bad_init_msg);
    return (msd);
}


// Initiate simulation.
//
bool
cSpiceIPC::runnit(const char *what)
{
    ipc_sigint_back = signal(SIGINT, interrupt_hdlr);

    DSPpkg::self()->SetWorking(true);
    SCD()->PopUpSim(SpBusy);
    if (!write_msg(what, ipc_msg_skt)) {
        DSPpkg::self()->SetWorking(false);
        return (false);
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ipc_msg_skt, &readfds);
    timeval to;
    to.tv_sec = 0;
    to.tv_usec = 0;
    int i = select(ipc_msg_skt+1, &readfds, 0, 0, &to);
    if (i > 0) {
        if (!complete_spice()) {
            SCD()->PopUpSim(SpError);
            Menu()->MenuButtonSet(MMmain, MenuRUN, false);
        }
    }
    else {
        ipc_in_spice = true;
#ifdef WIN32
        setup_async_io(true);
#endif
        if (!set_async(ipc_msg_skt, true))
            Log()->WarningLog(SpiceIPC, Errs()->get_error());
    }
    DSPpkg::self()->SetWorking(false);
    return (true);
}


// Write str to the socket, handle errors and partials.
//
bool
cSpiceIPC::write_msg(const char *str, int fd)
{
    if (!str)
        return (true);
    if (fd < 0)
        return (false);
    if (netdbg())
        fprintf(stderr, "xic: sending \"%s\"\n", str);
    int len = strlen(str) + 1;

    int bytes = 0;
    for (;;) {
        int i = send(fd, str, len - bytes, 0);
        if (i < 0) {
            Errs()->sys_error("write_msg: send");
            Errs()->add_error("write_msg: connection broken.");
            close_all();
            return (false);
        }
        bytes += i;
        if (bytes == len)
            break;
        str += i;
    }
    if (netdbg())
        fprintf(stderr, "xic: done\n");
    return (true);
}


// Return true if fd is ready for read.  Return false if timeout or
// error, with a message in the Errs system.  If silent, don't add a
// timeout message.  Arg timeout_ms is timeout time in milliseconds.
//
bool
cSpiceIPC::isready(int fd, int timeout_ms, bool silent)
{
    if (fd < 0)
        return (false);
    bool waitmode = false;
    if (timeout_ms <= 0) {
        waitmode = true;  // block forever.
        timeout_ms = 50;
    }

    timeval to;
    to.tv_sec = timeout_ms/1000;
    to.tv_usec = (timeout_ms % 1000)*1000;

    unsigned int t0 = Tvals::millisec();
    for (;;) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
#ifdef WIN32
        int i = select(fd+1, &readfds, 0, 0, &to);
#else
        // Back out of any SIGALRM handler, since it may break
        // the select timeout.
        sig_t hdlr = signal(SIGALRM, SIG_IGN);
        int i = select(fd+1, &readfds, 0, 0, &to);
        signal(SIGALRM, hdlr);
#endif
        if (i < 0) {
            if (errno == EINTR) {
                if (waitmode)
                    continue;
                unsigned int t = Tvals::millisec();
                timeout_ms -= (t - t0);
                t0 = t;
                if (timeout_ms <= 0)
                    i = 0;
                else {
                    to.tv_sec = timeout_ms/1000;
                    to.tv_usec = (timeout_ms % 1000)*1000;
                    continue;
                }
            }
            else {
                Errs()->sys_error("isready: select");
                return (false);
            }
        }
        if (i == 0) {
            if (waitmode) {
                if (DSPpkg::self()->CheckForInterrupt()) {
                    // ^C was typed in Xic graphics window.
                    if (fd == ipc_msg_skt)
                        InterruptSpice();
                }
                continue;
            }
            if (!silent)
                Errs()->add_error("isready: Connection timed out.");
            return (false);
        }
        if (FD_ISSET(fd, &readfds))
            break;
    }
    return (true);
}


// Read a message, and return the text in bufp.  Read until a zero
// byte, handle errors.  If timeout is 0, the operation is expected to
// take a while, such as a simulation.  In this case, handle
// interrupts while waiting.
//
// If line_brk is true, return when '\n' seen.
//
// If databuf is not null, certain interface commands that return
// binary data will create a data block.
//
bool
cSpiceIPC::read_msg(int fd, int timeout, char **bufp, bool line_brk,
    unsigned char **databuf)
{
    if (bufp)
        *bufp = 0;
    if (databuf)
        *databuf = 0;

    if (!isready(fd, timeout*1000)) {
        close_all();
        return (false);
    }

    sLstr lstr;
    char prev_char = 0;
    for (;;) {
        char c;
        int i = recv(fd, &c, 1, 0);
        if (i <= 0) {
            if (i < 0) {
                if (errno == EINTR)
                    continue;
                Errs()->sys_error("read_msg: recv");
            }
            Errs()->add_error("read_msg: Connection broken.");
            close_all();
            return (false);
        }

        // Messages from WRspice are 0 terminated.
        if (c == 0)
            break;
        if (c == '\n') {
            if (line_brk)
                // Set when reading WRspice banner strings, which have no
                // termination other than newline.
                break;
            if (prev_char == '\r')
                // Also return on network termination.
                break;
        }
        if (c >= ' ')
            lstr.add_c(c);
        prev_char = c;
    }
    if (netdbg())
        fprintf(stderr, "xic: received \"%s\"\n",
            lstr.string() ? lstr.string() : "");

    if (bufp) {
        char *t = lstr.string_trim();
        if (!t)
            t = lstring::copy("");
        *bufp = t;
    }

    if (lstring::match("data", lstr.string())) {
        // If the message is "data numbytes", binary data will follow.
        int nbytes = atoi(lstr.string() + 5);
        if (nbytes > 0) {
            unsigned char *dbuf = new unsigned char[nbytes];
            unsigned char *p = dbuf;
            while (nbytes) {
                int i = recv(fd, (char*)p, nbytes, 0);
                if (i <= 0) {
                    if (i < 0) {
                        if (errno == EINTR)
                            continue;
                        Errs()->sys_error("read_msg: recv");
                    }
                    Errs()->add_error("read_msg: Connection broken.");
                    close_all();
                    delete [] dbuf;
                    return (false);
                }
                nbytes -= i;
                p += i;
            }
            if (databuf)
                *databuf = dbuf;
            else
                delete [] dbuf;
        }
    }
    return (true);
}


// Read the return from a WRspice command, used in DoCmd.  Neither
// socket is set for async i/o.  This will block until a return is
// received or an error occurs, periodically checking for interrupts.
//
// If databuf is not null, certain interface commands that return
// binary data will create a data block.
//
bool
cSpiceIPC::read_cmd_return(char **msgbuf,  char **outbuf,
    unsigned char **databuf)
{
    if (msgbuf)
        *msgbuf = 0;
    if (outbuf)
        *outbuf = 0;
    if (databuf)
        *databuf = 0;

    if (ipc_msg_skt < 0)
        return (false);

    int tfd = ipc_msg_skt;
    if (ipc_stdout_skt > tfd)
        tfd = ipc_stdout_skt;

    sLstr outlstr;

    // Wait for a return on the msg socket, while waiting periodically
    // check for interrupts and grab any stdout that comes along.

    for (;;) {
        timeval to;
        to.tv_sec = 0;
        to.tv_usec = 100000;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(ipc_msg_skt, &readfds);
        if (ipc_stdout_skt > 0)
            FD_SET(ipc_stdout_skt, &readfds);
#ifdef WIN32
        int i = select(tfd+1, &readfds, 0, 0, &to);
#else
        // Back out of any SIGALRM handler, since it may break
        // the select timeout.
        sig_t hdlr = signal(SIGALRM, SIG_IGN);
        int i = select(tfd+1, &readfds, 0, 0, &to);
        signal(SIGALRM, hdlr);
#endif
        if (i < 0) {
            if (errno == EINTR)
                continue;
            else {
                Errs()->sys_error("read_cmd_return: select");
                close_all();
                return (false);
            }
        }
        if (i == 0) {
            if (DSPpkg::self()->CheckForInterrupt()) {
                // ^C was typed in Xic graphics window.
                InterruptSpice();
            }
            continue;
        }
        if (ipc_stdout_skt > 0 && FD_ISSET(ipc_stdout_skt, &readfds)) {
            for (;;) {
                char c;
                i = recv(ipc_stdout_skt, &c, 1, 0);
                if (i == 1) {
                    outlstr.add_c(c);
                    if (c == '\n')
                        break;
                }
                else {
                    if (i < 0 && errno == EINTR)
                        continue;
                    Errs()->sys_error("read_cmd_return: recv");
                    close_all();
                    return (false);
                }
            }
            continue;
        }
        if (ipc_msg_skt < 0)
            return (false);

        if (FD_ISSET(ipc_msg_skt, &readfds))
            break;
    }

    sLstr lstr;
    char prev_char = 0;
    for (;;) {
        char c;
        int i = recv(ipc_msg_skt, &c, 1, 0);
        if (i <= 0) {
            if (i < 0) {
                if (errno == EINTR)
                    continue;
                Errs()->sys_error("read_cmd_return: recv");
            }
            Errs()->add_error("read_cmd_return: Connection broken.");
            close_all();
            return (false);
        }

        // Messages from WRspice are 0 terminated.
        if (c == 0)
            break;
        if (c == '\n') {
            if (prev_char == '\r')
                // Also return on network termination.
                break;
        }
        if (c >= ' ')
            lstr.add_c(c);
        prev_char = c;
    }
    if (netdbg())
        fprintf(stderr, "xic: received \"%s\"\n",
            lstr.string() ? lstr.string() : "");

    if (msgbuf) {
        char *t = lstr.string_trim();
        if (!t)
            t = lstring::copy("");
        *msgbuf = t;
    }

    if (lstring::match("data", lstr.string())) {
        // If the message is "data numbytes", binary data will follow.
        int nbytes = atoi(lstr.string() + 5);
        if (nbytes > 0) {
            unsigned char *dbuf = new unsigned char[nbytes];
            unsigned char *p = dbuf;
            while (nbytes) {
                int i = recv(ipc_msg_skt, (char*)p, nbytes, 0);
                if (i <= 0) {
                    if (i < 0) {
                        if (errno == EINTR)
                            continue;
                        Errs()->sys_error("read_cmd_return: recv");
                    }
                    Errs()->add_error("read_cmd_return: Connection broken.");
                    close_all();
                    delete [] dbuf;
                    return (false);
                }
                nbytes -= i;
                p += i;
            }
            if (databuf)
                *databuf = dbuf;
            else
                delete [] dbuf;
        }
    }

    // Do a final read on the stdout, in case there is something left.
    char *tbf = read_stdout(200);
    if (tbf) {
       outlstr.add(tbf);
       delete [] tbf;
    }
    if (outbuf)
        *outbuf = outlstr.string_trim();

    return (true);
}


// Drain the stdout channel.
//
char *
cSpiceIPC::read_stdout(int timeout_ms)
{
    // Read the WRspice stdout/stderr.
    sLstr lstr;
    if (ipc_stdout_skt > 0) {
        bool done = false;
        while (!done && isready(ipc_stdout_skt, timeout_ms, true)) {
            char c;
            while (!done) {
                int i = recv(ipc_stdout_skt, &c, 1, 0);
                if (i == 1) {
                    lstr.add_c(c);
                    if (c == '\n')
                        break;
                }
                else {
                    if (i < 0 && errno == EINTR)
                        continue;
                    // Read error.  This probably means that WRspice died.
                    Errs()->add_error("Read error, stdio connection broken.");
                    close_all();
                    done = true;
                }
            }
            timeout_ms = 100;
        }
    }
    return (lstr.string_trim());
}


void
cSpiceIPC::dump_stdout()
{
    // Read the messages from WRspice, if any.

    if (ipc_stdout_skt && isready(ipc_stdout_skt, 500, true)) {
        for (;;) {
            char c;
            int i = recv(ipc_stdout_skt, &c, 1, 0);
            if (i == 1) {
                fputc(c, stderr);
            }
            else {
                if (i < 0 && errno == EINTR)
                    continue;
                // Read error.  This probably means that WRspice died.
                Errs()->add_error("Read error, stdio connection broken.");
                close_all();
                break;
            }
            if (ipc_stdout_skt < 0)
                break;

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(ipc_stdout_skt, &readfds);
            timeval to;
            to.tv_sec = 0;
            to.tv_usec = 50;
#ifdef WIN32
            i = select(ipc_stdout_skt+1, &readfds, 0, 0, &to);
#else
            sig_t hdlr = signal(SIGALRM, SIG_IGN);
            i = select(ipc_stdout_skt+1, &readfds, 0, 0, &to);
            signal(SIGALRM, hdlr);
#endif
            if (i <= 0)
                break;
        }
    }

    if (Errs()->has_error())
        fprintf(stderr, "%s\n", Errs()->get_error());
}


// Send str to the WRspice process as command input.  Wait for
// response.  Return the response obtained, 0 if error.
//
// The return needs to be freed.
//
// BEWARE:  If the ipc_stdout_skt is in synchronous mode, and the
// debugging is turned off, and the command in write_message dumps a
// lot of stdout (such as an ascii plot) the program will hang when
// the internal buffer is full.  The write_msg call should be
// followed by read_msg on the ipc_stdout_skt to avoid this.
//
char *
cSpiceIPC::send_to_spice(const char *str, int timeout)
{
    if (ipc_msg_skt >= 0 && str) {
        if (netdbg()) {
            // Do the transaction synchronously to maintain sane
            // ordering of debugging output lines.

            if (!set_async(ipc_stdout_skt, false))
                Errs()->get_error();
            bool ok = write_msg(str, ipc_msg_skt);
            if (ok)
                dump_stdout();
            if (!set_async(ipc_stdout_skt, true))
                Errs()->get_error();
            if (!ok)
                return (0);
        }
        else if (!write_msg(str, ipc_msg_skt))
            return (0);
        char *tbf = 0;
        if (!read_msg(ipc_msg_skt, timeout, &tbf))
            return (0);
        return (tbf);
    }
    return (0);
}


// Send the deck to WRspice, verbatim.  The first string must be
// "decksource".  The terminating '@' is appended.  Return false if
// source fails or transmission error.  Output contains the
// stdout/stderr if any.
//
bool
cSpiceIPC::deck_to_spice(stringlist *deck, char **outbuf)
{
    if (outbuf)
        *outbuf = 0;
    if (!deck || strcmp(deck->string, "decksource")) {
        Errs()->add_error("deck_to_spice: Bad deck, magic keyword not found.");
        return (false);
    }

    // Switch stdout/stderr back to synchronous mode, since we
    // explicitly read the stdout/stderr return.
    //
    if (!set_async(ipc_stdout_skt, false))
        Log()->WarningLog(SpiceIPC, Errs()->get_error());

    bool ok = true;
    for (stringlist *s = deck; s; s = s->next) {
        ok = write_msg(s->string, ipc_msg_skt);
        if (!ok)
            break;
    }
    // Write termination.
    if (ok && !write_msg("@", ipc_msg_skt))
        ok = false;
    if (ok) {
        char *tbf = 0;
        if (!read_msg(ipc_msg_skt, 0, &tbf))
            ok = false;
        else if (tbf[0] == 'e' && tbf[1] == 'r') {
            // Source failed.
            Errs()->add_error("deck_to_spice: source failed.");
            ok = false;
        }
        delete [] tbf;

        // Grab the stdout/stderr, may contain an error message, or
        // command output.
        tbf = read_stdout(500);
        if (outbuf)
            *outbuf = tbf;
        else
            delete [] tbf;
    }

    // Return to asynchronous stdout/stderr.
    if (!set_async(ipc_stdout_skt, true))
        Log()->WarningLog(SpiceIPC, Errs()->get_error());

    return (ok);
}


// Determine whether there is a run in progress.
//
bool
cSpiceIPC::inprogress()
{
    if (ipc_in_spice)
        return (true);
    char *tbf = send_to_spice("inprogress", 120);
    char c = tbf ? *tbf : 0;
    delete [] tbf;
    return (c == 'y');
}


// Clean up after run completes.
//
bool
cSpiceIPC::complete_spice()
{
    char *tbf;
    if (!read_msg(ipc_msg_skt, 0, &tbf) || ipc_msg_skt < 0)
        return (false);
    delete [] tbf;
    tbf = send_to_spice("curckt", 120);
    if (!tbf)
        return (false);
    if (*tbf) {
        delete [] ipc_last_cir;
        ipc_last_cir = lstring::copy(tbf);
    }
    delete [] tbf;
    tbf = send_to_spice("curplot", 120);
    if (!tbf)
        return (false);
    if (*tbf) {
        delete [] ipc_last_plot;
        ipc_last_plot = lstring::copy(tbf);
    }
    delete [] tbf;

    if (!inprogress()) {
        if (ipc_msg_skt < 0)
            return (false);
        ClearIplot(true);
        if (ipc_msg_skt < 0)
            return (false);

        SCD()->PopUpSim(SpDone);
        Menu()->MenuButtonSet(MMmain, MenuRUN, false);
    }
    else {
        SCD()->PopUpSim(SpPause);
        Menu()->MenuButtonSet(MMmain, MenuRUN, false);
    }
    return (true);
}


// Close all open sockets and perform general cleanup.  This is called
// if comm error or child termination, use CloseSpice to shut down
// gracefully.
//
void
cSpiceIPC::close_all()
{
    if (ipc_msg_skt > 0) {
        CLOSESOCKET(ipc_msg_skt);
        ipc_msg_skt = -1;
    }
    if (ipc_stdout_skt > 0) {
        CLOSESOCKET(ipc_stdout_skt);
        ipc_stdout_skt = -1;
    }
    if (ipc_stdout_skt2 > 0) {
        CLOSESOCKET(ipc_stdout_skt2);
        ipc_stdout_skt2 = -1;
    }
#ifdef WIN32
    if (ipc_stdout_hrd) {
        CloseHandle(ipc_stdout_hrd);
        ipc_stdout_hrd = 0;
    }
#endif

    setup_async_io(false);
#ifdef SIGPIPE
    if (ipc_level == 1)
        signal(SIGPIPE, ipc_sigpipe_back);
#endif

    // Clean up and reinit for next time.
    delete [] ipc_analysis;
    delete [] ipc_last_cir;
    delete [] ipc_last_plot;
    delete [] ipc_spice_path;
    delete [] ipc_spice_host;
    delete [] ipc_host_prog;
    ctor_init();
}


namespace {
    // Handler for asynchronous read.
    //
    void
    sigio_hdlr(int sig)
    {
        SCD()->spif()->SigIOhdlr(sig);
    }
}


// In Unix/Linux switch to/from our SIGIO handler.  The handler
// processes messages and stdout/stderr from WRspice while a
// connection is active.  In Windows, start the polling thread.
//
void
cSpiceIPC::setup_async_io(bool async)
{
#ifdef WIN32
    if (async) {
        if (ipc_level == 1) {
            ipc_level = 2;
            _beginthread((void(*)(void*))(void*)sigio_hdlr, 0, 0);
        }
    }
    else if (ipc_level == 2) {
        ipc_level = 1;
    }
#else
    if (async) {
        if (ipc_level == 1) {
            ipc_sigio_back = signal(SIGIO, sigio_hdlr);
            ipc_level = 2;
        }
    }
    else if (ipc_level == 2) {
        signal(SIGIO, ipc_sigio_back);
        ipc_level = 1;
    }
#endif
}


// Switch fd to/from asynchronous mode.
//
bool
cSpiceIPC::set_async(int fd, bool async)
{
#ifdef WIN32
    if (fd == ipc_stdout_skt)
        ipc_stdout_async = async;
#else
    if (fd < 0)
        return (true);
    if (async) {
        // set process group
        if (fd == ipc_msg_skt)
            ipc_msg_pgid = fcntl(fd, F_GETOWN, 0);
        else
            ipc_stdout_pgid = fcntl(fd, F_GETOWN, 0);
        if (fcntl(fd, F_SETOWN, getpid()) < 0) {
            Errs()->sys_error("set_async: fcntl");
            return (false);;
        }
        int flags = fcntl(fd, F_GETFL, 0);
        flags |= FASYNC;
        if (fcntl(fd, F_SETFL, flags) == -1) {
            Errs()->sys_error("set_async: fcntl");
            signal(SIGIO, ipc_sigio_back);
            return (false);
        }
    }
    else {
        int flags = fcntl(fd, F_GETFL, 0);
        flags &= ~FASYNC;
        if (fcntl(fd, F_SETFL, flags) == -1) {
            Errs()->sys_error("set_async: fcntl");
            return (false);
        }
        if (fd == ipc_msg_skt)
            fcntl(fd, F_SETOWN, ipc_msg_pgid);
        else
            fcntl(fd, F_SETOWN, ipc_stdout_pgid);
    }
#endif
    return (true);
}


#ifdef DEMO_EXPORT
#else

// If word is non-null, prepend it to the deck.  Expand all .inc/.lib
// type lines.  On success, a new expanded deck is returned (the
// original deck is freed).  On error, false is returned, and the
// original deck is retained.
//
bool
cSpiceIPC::expand_includes(stringlist **deck, const char *word)
{
    sLibMap lmap;
    return (lmap.expand_includes(deck, word));
}

#endif


// Static private function.
// This serves the function of kill(spicepid, SIGTERM).
//
void
cSpiceIPC::terminate_spice(int pid)
{
    if (!pid)
        return;
#ifdef HAVE_KILL
    kill(pid, SIGTERM);
#else
#ifdef WIN32
    char buf[64];
    snprintf(buf, sizeof(buf), "wrspice.sigterm.%d", pid);
    HANDLE hs = OpenSemaphore(SEMAPHORE_MODIFY_STATE, false, buf);
    if (hs) {
        long pv;
        ReleaseSemaphore(hs, 1, &pv);
        CloseHandle(hs);
    }
#endif
#endif
}


#ifdef WIN32

// Static private function.
// Stdout/stderr thread proc for remote mode, echo chars to stdout in
// async mode, just hold otherwise.
//
void
cSpiceIPC::remote_stdout_thread_proc(void *arg)
{
    cSpiceIPC *sp = (cSpiceIPC*)arg;
    for (;;) {
        if (sp->ipc_stdout_skt < 0)
            break;
        timeval to;
        to.tv_sec = 0;
        to.tv_usec = 200000;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sp->ipc_stdout_skt, &readfds);
        int i = select(sp->ipc_stdout_skt+1, &readfds, 0, 0, &to);
        if (i < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (i == 0)
            continue;

        if (sp->ipc_stdout_async) {
            char c;
            i = recv(sp->ipc_stdout_skt, &c, 1, 0);
            if (i == 1) {
                fputc(c, stdout);
                fflush(stdout);
            }
            else if (errno == EINTR)
                continue;
            else
                break;
        }
        else    
            Sleep(200);
    }
}


// Static private function.
// Stdout/stderr thread proc for local mode, echo chars to stdout in
// async mode, to the socket otherwise.
//
void
cSpiceIPC::local_stdout_thread_proc(void *arg)
{
    bool need_char = true;
    cSpiceIPC *sp = (cSpiceIPC*)arg;
    for (;;) {
        char c;
        if (!sp->ipc_stdout_hrd)
            break;
        if (need_char) {
            DWORD nb;
            if (!ReadFile(sp->ipc_stdout_hrd, &c, 1, &nb, 0) || !nb)
                break;
        }
        if (sp->ipc_stdout_async) {
            fputc(c, stdout);
            fflush(stdout);
        }
        else {
            if (sp->ipc_stdout_skt2 < 0)
                break;
            if (send(sp->ipc_stdout_skt2, &c, 1, 0) != 1) {
                if (errno == EINTR) {
                    need_char = false;
                    continue;
                }
                break;
            }
        }
        need_char = true;
    }
}


#ifndef DEMO_EXPORT
namespace {
    int msg_proc(void *arg)
    {
        char *s = (char*)arg;
        DSPmainWbag(PopUpMessage(s, false))
        delete [] s;
        return (0);
    }
}
#endif


// Static private function.
// Inform when child process terminates.
//
void
cSpiceIPC::child_thread_proc(void *arg)
{
    PROCESS_INFORMATION *info = (PROCESS_INFORMATION*)arg;
    HANDLE h = info->hProcess;
    UINT pid = info->dwProcessId;
    delete info;
    if (WaitForSingleObject(h, INFINITE) == WAIT_OBJECT_0) {
        DWORD status;
        GetExitCodeProcess(h, &status);
        char buf[128];
        snprintf(buf, sizeof(buf), "Child process %d exited ", pid);
        if (status) {
            int len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "with error status %ld.",
                status);
        }
        else
            strcat(buf, "normally.");

#ifndef DEMO_EXPORT
        // Can't pop up a window in this thread!  It dies when the
        // thread dies.
        DSPpkg::self()->RegisterIdleProc(msg_proc, lstring::copy(buf));
#endif
        SCD()->spif()->close_all();
    }
    CloseHandle(h);
}

#else

// Static private function.
// Inform when child process terminates.
//
void
cSpiceIPC::child_hdlr(int pid, int status, void*)
{
    char buf[128];
    *buf = '\0';
    if (WIFEXITED(status)) {
        snprintf(buf, sizeof(buf), "Child process %d exited ", pid);
        if (WEXITSTATUS(status)) {
            int len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "with error status %d.",
                WEXITSTATUS(status));
        }
        else
            strcat(buf, "normally.");
    }
    else if (WIFSIGNALED(status)) {
        snprintf(buf, sizeof(buf), "Child process %d exited on signal %d.",
            pid, WIFSIGNALED(status));
    }
    if (*buf) {
        DSPmainWbag(PopUpMessage(buf, false))
        SCD()->spif()->close_all();
    }
}

#endif


// Static private function.
// Interrupt signal handling for use during WRspice simulations.
//
void
cSpiceIPC::interrupt_hdlr(int)
{
    signal(SIGINT, interrupt_hdlr);
    SCD()->spif()->InterruptSpice();
}


// Static private function.
// Create socket and establish connection, return descriptor or
// -1 if error.
//
int
cSpiceIPC::open_skt(hostent *he, int port)
{
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        Errs()->sys_error("open_skt: socket");
        return (-1);
    }
    sockaddr_in skt;
    memset(&skt, 0, sizeof(sockaddr_in));
    memcpy(&skt.sin_addr, he->h_addr, he->h_length);
    skt.sin_family = AF_INET;
    skt.sin_port = htons(port);
    if (connect(sd, (sockaddr*)&skt, sizeof(skt)) < 0) {
        Errs()->sys_error("open_skt: connect");
        CLOSESOCKET(sd);
        return (-1);
    }
    return (sd);
}


#ifdef DEMO_EXPORT

// The function below must be implemented in user code.  The function
// sets the basic strings and flags to configure the interface.

#else

// This is the Xic implementation.

namespace {
    char *add_exe(char *program)
    {
#ifdef WIN32
        // The file must have a .exe extension, .bat files do not
        // work.  The user must load this from xictools/wrspice/bin
        // since the .exe is not exported to xictools/bin.  The
        // default path is obtained from msw::GetProgramRoot so should
        // be ok.

        if (!program)
            return (0);
        char *t = strrchr(program, '.');
        if (t && lstring::cieq(t, ".exe"))
            return (program);
        char *prg = new char[strlen(program) + 5];
        strcpy(prg, program);
        t = strrchr(prg, '.');
        if (t && lstring::cieq(t, ".bat"))
            *t = 0;
        strcat(prg, ".exe");
        delete [] program;
        program = prg;

#endif
        return (program);
    }
}


bool
cSpiceIPC::callback(const char **spice_path, const char **spice_host,
    const char **host_prog, bool *no_graphics, bool *no_toolbar,
    const char **remote_display)
{
    static char *path;

    // Find the SPICE path, for local mode.
    //
    delete [] path;
    path = 0;
    const char *pg = CDvdb()->getVariable(VA_SpiceProg);
    if (pg && *pg)
        path = add_exe(lstring::copy(pg));
    else {
        // Find the name of the executable.  A default is set in startup
        // code, but can be overridden by the variable.
        pg = CDvdb()->getVariable(VA_SpiceExecName);
        if (!pg || !*pg)
            pg = XM()->SpiceExecName();
        if (pg && *pg) {
            char *pgname = lstring::copy(pg);
            pgname = add_exe(pgname);

            // Find the executables directory.  A default value is set
            // in startup code, but can be overridden by the variable.
            const char *dir = CDvdb()->getVariable(VA_SpiceExecDir);
            if (!dir || !*dir)
                dir = XM()->ExecDirectory();
            if (dir && *dir)
                path = add_exe(pathlist::mk_path(dir, pgname));
        }
    }
    *spice_path = path;

    // Find the SPICE host for remote (wrspiced) mode.  If this is not
    // null or empty, the interface will use remote mode.  Otherwise,
    // local mode will be used.
    //
    const char *host = CDvdb()->getVariable(VA_SpiceHost);
    if (!host)
        host = getenv("SPICE_HOST");
    *spice_host = host;

    // Find SPICE executable name on remote host.
    //
    const char *pgname = CDvdb()->getVariable(VA_SpiceExecName);
    if (!pgname || !*pgname)
        pgname = XM()->SpiceExecName();
    *host_prog = pgname;

    // Set X graphics usage.
    //
    *no_graphics = false;

    // Set WRspice toolbar display flag.
    //
    *no_toolbar = CDvdb()->getVariable(VA_NoSpiceTools);

    // The DISPLAY name to use for the X connection on the remote
    // machine, overrides the assumed local display name when
    // non-null.  This applies to remote connections (using wrspiced)
    // only, and only when using graphics.
    //
    // In legacy X-window systems, the display name would typically be
    // in the form hostname:0.0.  A remote system will draw to the
    // local display if the local hostname was used in the display
    // name, and the local X server permissions were set (with
    // xauth/xhost) to allow access.  Typically, the user would log in
    // to a remote system with telnet or ssh, set the DISPLAY
    // variable, perhaps give "xhost +" on the local machine, then run
    // X programs.
    //
    // This method has been largely superseded by use of "X
    // forwarding" in ssh.  This is often automatic, or may require
    // the '-X' option on the ssh command line.  In this case, after
    // using ssh to log in to the remote machine, the DISPLAY variable
    // is automatically set to display on the local machine.  X
    // applications "just work", with no need to fool with the DISPLAY
    // variable, or permissions.
    //
    // The present Xic remote access code does not know about the ssh
    // protocol, so we have to fake it in some cases.  In most cases
    // the older method will still work.
    //
    // The ssh protocol works by setting up a dummy display, with a
    // name something like "localhost:10.0", which in actuality
    // connects back to the local display.  Depending on how many ssh
    // connections are currently in force, the "10" could be "11",
    // "12", etc.
    //
    // In the present case, if we want to use ssh for X transmission,
    // the display name used here must match an existing ssh display
    // name on the remote system that maps back to the local display.
    //
    // If there is an existing ssh connection to the remote machine,
    // the associated DISPLAY can be used.  If there is no existing
    // ssh connection, one can be established, and that used.  E.g.,
    // from the ssh window, type "echo $DISPLAY" and use the value
    // printed.
    //
    // The display name provided here will override the assumed
    // display name created internally with the local host name.
    //
    *remote_display = CDvdb()->getVariable(VA_SpiceHostDisplay);

    return (true);
}

#endif


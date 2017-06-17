
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: onexit.cc,v 5.39 2015/05/12 03:59:55 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "menu.h"
#include "events.h"
#include "promptline.h"
#include "errorlog.h"
#include "filestat.h"
#include "miscutil.h"
#include "secure.h"

#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef WIN32
#include "msw.h"
#endif


// Confirm quit after interrupt.  This desensitizes all main-frame menus
// during the prompt.  If the user elects to continue, the interrupt
// flag is reset.
//
bool
cMain::ConfirmAbort(const char *msg)
{
    if (DSP()->Interrupt() == DSPinterSys) {
        DSP()->SetInterrupt(DSPinterNone);
        return (true);
    }
    if (DSP()->Interrupt() == DSPinterUser) {
        DSP()->SetInterrupt(DSPinterNone);
        if (XM()->RunMode() != ModeNormal) {
            fprintf(stderr, "Interrupted!\n");
            return (true);
        }
        if (xm_no_confirm_abort) {
            PL()->ShowPrompt("Interrupted!");
            return (true);
        }
        Menu()->SetSensGlobal(false);
        dspPkgIf()->SetOverrideBusy(true);
        char *in = PL()->EditPrompt(
            (msg ? msg : "Interrupted.  Abort? "), "n");
        Menu()->SetSensGlobal(true);
        dspPkgIf()->SetOverrideBusy(false);
        in = lstring::strip_space(in);
        if (in && (*in == 'y' || *in == 'Y')) {
            PL()->ShowPrompt("Aborted.");
            return (true);
        }
        PL()->ShowPrompt("Continuing...");
        SI()->ClearInterrupt();
    }
    return (false);
}


// Exit the application.
//
void
cMain::Exit(ExitType exit_type)
{
    if (exit_type == ExitCheckMod) {
        static bool here;
        if (here)
            return;
        here = true;
        EV()->InitCallback();
        char *in;
        switch (CheckModified(false)) {
        case CmodAborted:
            PL()->ShowPrompt("Exit ABORTED");
            break;
        case CmodFailed:
            in = PL()->EditPrompt(
                "Error: write FAILED, there are unsaved cells.  Exit anyway? ",
                "n");
            in = lstring::strip_space(in);
            if (in && (*in == 'y' || *in == 'Y'))
                Exit(ExitNormal);
            PL()->ErasePrompt();
            break;
        case CmodOK:
        case CmodNoChange:
            Exit(ExitNormal);
            break;
        }
        here = false;
        return;
    }
    if (exit_type == ExitNormal) {
        xm_exit_cleanup_done = true;
        ScedIf()->closeSpice();
        char *s = getenv("XIC_EXIT_CMD");
        if (s) {
            s = lstring::copy(s);
            system(s);
        }
        Auth()->closeValidation();
        Log()->CloseLogDir();
        dspPkgIf()->Halt();
        exit(0);
    }
    else if (exit_type == ExitPanic) {
        static bool in_panic;
        xm_exit_cleanup_done = true;
        if (in_panic) {
            fputs("Hard panic, couldn't save cells.\n", stderr);
#ifdef SIGKILL
            raise(SIGKILL);
#endif
            exit(1);
        }
        in_panic = true;
        ScedIf()->closeSpice();
        Auth()->closeValidation();
        filestat::delete_files();

        signal(SIGINT, SIG_DFL);
        if (DSP()->CurCellName()) {
            switch (CheckModified(true)) {
            case CmodAborted:
            case CmodFailed:
                fputs("Error occurred when dumping cells.\n", stderr);
                if (xm_panic_fp) {
                    fputs("*** Error occurred when dumping cells.\n",
                        xm_panic_fp);
                    fputs("Check xic_panic.log file.\n", stderr);
                    fclose(xm_panic_fp);
                    xm_panic_fp = 0;
                }
                break;
            case CmodOK:
            case CmodNoChange:
                if (xm_panic_dir) {
                    fprintf(stderr,
                        "All modified cells have been dumped in ./%s.\n",
                        xm_panic_dir);
                    if (xm_panic_fp) {
                        fprintf(xm_panic_fp,
                            "All modified cells have been dumped in ./%s.\n",
                            xm_panic_dir);
                    }
                }
                if (xm_panic_fp) {
                    fclose(xm_panic_fp);
                    xm_panic_fp = 0;
                }
                break;
            }
        }
#ifdef SIGKILL
        raise(SIGKILL);
#endif
        exit(1);
    }
    else if (exit_type == ExitDebugger) {
        char header[256];
        sprintf(header, "%s-%s %s (%s)", Product(), VersionString(),
            OSname(), TagString());

        const char *logdir = Log()->LogDirectory();
        extern void* DeathAddr;
        miscutil::dump_backtrace(Program(), header, logdir, DeathAddr);
        if (logdir && *logdir)
            fprintf(stderr, "All logs retained in %s.\n", logdir);
        if (access(GDB_OFILE, F_OK) == 0) {
            if (!getenv("XICNOMAIL") && !getenv("XTNOMAIL")) {
                bool ret = miscutil::send_mail(Log()->MailAddress(),
                    "Xic crash report", "", GDB_OFILE);
                if (ret)
                    fprintf(stderr, "File %s emailed to %s.\n", GDB_OFILE,
                        Log()->MailAddress());
                else
                    fprintf(stderr, "Failed to email %s file to %s, please"
                        " send manually.\n", GDB_OFILE,
                        Log()->MailAddress());
            }
            else
                fprintf(stderr, "Please email %s file to %s.\n", GDB_OFILE,
                    Log()->MailAddress());
        }
        Exit(ExitPanic);
    }
}


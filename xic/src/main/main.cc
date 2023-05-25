
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
#include "main.h"
#include "fio.h"
#include "editif.h"
#include "drcif.h"
#include "extif.h"
#include "dsp_layer.h"
#include "dsp_tkif.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_daemon.h"
#include "tech.h"
#include "cvrt.h"
#include "ghost.h"
#include "events.h"
#include "errorlog.h"
#include "promptline.h"
#include "pushpop.h"
#include "layertab.h"
#ifdef HAVE_SECURE
#include "secure.h"
#endif
#include "miscutil/timer.h"
#include <time.h>
#include <signal.h>

#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#endif


namespace { void timer_proc(); }

cMain *cMain::instancePtr = 0;

cMain::cMain()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cMain already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    xm_product                          = 0;
    xm_description                      = 0;
    xm_program                          = 0;

    xm_program_root                     = 0;
    xm_prefix                           = 0;
    xm_tools_root                       = 0;
    xm_app_root                         = 0;
    xm_homedir                          = 0;

    xm_version_string                   = 0;
    xm_build_date                       = 0;
    xm_tag_string                       = 0;
    xm_about_file                       = 0;
    xm_os_name                          = 0;
    xm_arch                             = 0;
    xm_dist_suffix                      = 0;
    xm_geometry                         = 0;
    xm_tech_file_base                   = 0;
    xm_default_edit_name                = 0;
    xm_device_lib_name                  = 0;
    xm_model_lib_name                   = 0;
    xm_model_subdir_name                = 0;
    xm_init_script                      = 0;
    xm_startup_script                   = 0;
    xm_macro_file_name                  = 0;
    xm_exec_directory                   = 0;
    xm_spice_exec_name                  = 0;
    xm_font_file_name                   = 0;
    xm_logo_font_file_name              = 0;
    xm_log_name                         = 0;

    xm_auth                             = 0;
    xm_run_mode                         = ModeNormal;
    xm_product_code                     = 0;
    xm_build_year                       = 0;

    xm_bang_cmd_tab                     = 0;
    xm_var_bak                          = 0;
    xm_initial_mode                     = Physical;

    xm_push_data                        = 0;
    xm_debug_file                       = 0;
    xm_debug_fp                         = 0;
    xm_panic_fp                         = 0;
    xm_panic_dir                        = 0;

    xm_debug_flags                      = 0;
    xm_doing_help                       = false;
    xm_no_confirm_abort                 = false;
    xm_full_win_cursor                  = false;
    xm_app_init_done                    = false;
    xm_app_ready                        = false;
    xm_exit_cleanup_done                = false;
    xm_htext_cnames_only                = false;
    xm_mem_error                        = false;
    xm_saving_dev                       = false;
    xm_tree_captive                     = false;

    // instatiate core functionality
    new ErrRec;             // error handler
    new cCD;                // database
    new cGEO;               // computational geometry
    new cFIO;               // file i/o
    new cDisplay;           // windows
    setupInterface();
    new cTech;              // techfile handling
    new SIparser;           // expression parser
    new SIinterp;           // script interpreter
    new cPushPop;           // push/pop current cell

    InitializeStrings();
    SetupVariables();
    LoadScriptFuncs();

    // instantiate helpers
    new cTimer;         // clock tick generator
    new cEventHdlr;     // keyboard/mouse events
    new cErrLog;        // error logging
    new cPromptLine;    // prompt line
    new cLayerTab;      // layer info
    new cGhost;         // ghost highlighting
    new cConvert;       // layout file i/o

    setupTech();        // setup techfile parser
    setupHcopy();       // setup hardcopy interface
    setupBangCmds();    // set up text '!' commands

    Timer()->register_callback(timer_proc);
}


// Private static error exit.
//
void
cMain::on_null_ptr()
{
    fprintf(stderr, "Singleton class cMain used before instantiated.\n");
    exit(1);
}


// Return static string containing version data and date.
//
const char *
cMain::IdString()
{
    // find re-release number, if any
    int dcnt = 0;
    const char *sr = TagString();
    while (*sr) {
        if (*sr == '-') {
            dcnt++;
            if (dcnt == 4)
                break;
        }
        sr++;
    }

    static char buf[64];
    time_t tloc = time(0);
    struct tm now = *gmtime(&tloc);
    const char *s = lstring::strip_path(Program());
    snprintf(buf, sizeof(buf), "%s %s%s %s %s %02d/%02d/%04d %02d:%02d GMT", s,
        VersionString(), sr, OSname(), Arch(), now.tm_mon + 1, now.tm_mday,
        now.tm_year + 1900, now.tm_hour, now.tm_min);
    return (buf);
}


//
// The interface to the daemon, for server mode.
//

namespace {
    struct daemon_interface : public siDaemonIf
    {
        void app_clear() { XM()->ClearAll(false); }

        char *app_id_string()
            {
                char buf[128];
                snprintf(buf, sizeof(buf), "%s-%s", XM()->Product(),
                    XM()->VersionString());
                return (lstring::copy(buf));
            }

        void app_listen_init()
            { Timer()->start(getenv("XIC_NOTIMER") ? 0 : 1000); }

        void app_transact_init() { DSP()->SetInterrupt(DSPinterNone); }

        FILE *app_open_log(const char *name, const char *mode)
            {
                if (!name || !mode)
                    return (0);
                if (*mode == 'a')
                    return (Log()->OpenLog(name, mode, true));
                return (Log()->OpenLog(name, mode));
            }
    };
}


int
cMain::Daemon(int port)
{
    daemon_interface dif;
    return (siDaemon::start(port, &dif));
}
// End of cMain functions.


//-----------------------------------------------------------------------------
// The heartbeat timer callback

namespace {
    unsigned long die_when;

#ifdef HAVE_SECURE
    int die_timeout(void*)
    {
        XM()->Exit(ExitPanic);
        // Above shouldn't return.
#ifdef SIGKILL
        raise(SIGKILL);
#endif
        exit(1);
        return (false);
    }


    // Can't put the pop-up call in the handler, so use idle proc.
    //
    int v_proc(void *ptr)
    {
        DSPpkg::self()->ErrPrintf(ET_ERROR, (char*)ptr);
        delete [] (char*)ptr;
        DSPpkg::self()->RegisterTimeoutProc(AC_LIFETIME_MINUTES*60*1000,
            die_timeout, 0);
        return (0);
    }
#endif


    int m_proc(void*)
    {
        Log()->ErrorLog("exception",
            "Memory fault detected!  Info written to log file.\n");
        return (0);
    }


    void timer_proc()
    {
#ifdef HAVE_LOCAL_ALLOCATOR
        // The memory_busy flag is set in the allocator.  Allocating
        // memory when busy is a bad thing.

        if (Memory()->is_busy())
            return;
#endif
     
        if (die_when && Timer()->elapsed_msec() > die_when) {
            XM()->Exit(ExitPanic);
#ifdef SIGKILL
            raise(SIGKILL);
#endif
            exit(1);
        }
        if (XM()->MemError()) {
            XM()->SetMemError(false);
            if (XM()->RunMode() == ModeNormal)
                DSPpkg::self()->RegisterIdleProc(m_proc, 0);
        }
#ifdef HAVE_SECURE
        char *s = XM()->Auth()->periodicTest(Timer()->elapsed_msec()); 
        if (s) {
            if (XM()->RunMode() == ModeNormal)
                DSPpkg::self()->RegisterIdleProc(v_proc, s);
            else {
                delete [] s;
                die_when = Timer()->elapsed_msec() + 
                    AC_LIFETIME_MINUTES*60*1000;
            }
        }
#endif
    }
}


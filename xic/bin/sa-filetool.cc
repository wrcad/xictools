
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
 * filetool -- Integrated Circuit Layout Manipulation Tool                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "cd.h"
#include "fio.h"
#include "geo.h"
#include "filetool.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "miscutil/pathlist.h"
#include "miscutil/timer.h"
#include "miscutil/timedbg.h"

#include <signal.h>
#include <time.h>

#ifndef VERSION
#define VERSION "1.1"
#endif
#ifndef TIMELIM
// This will fail.
#endif


//
// This is a main function for the stand-alone filetool program.
//

// Instantiate needed components.
namespace {
    cTimer _tm_;  // Not really used, elapsed time always 0.
    cTimeDbg _tdb_;
    cCD _cd_;
    cFIO _fio_;
    cGEO _geo_;
    ErrRec _errs_;

    void init_signals();
    void exec_startup_script(const char*);
}


int
main(int argc, char **argv)
{
    time_t tx;
    time(&tx);
    if (tx > TIMELIM) {
        fprintf(stderr, "Sorry, demo time limit has expired.\n");
        return (1);
    }

    init_signals();
    exec_startup_script(".filetoolrc");

    cFileTool ft(VERSION);
    bool ret = ft.run(argc, argv);

    return (ret ? 0 : 127);
}


namespace {
    // The signal handler.
    //
    void
    sig_hdlr(int sig)
    {
        signal(sig, sig_hdlr);  // reset for SysV

        if (sig == SIGINT) {
        }
        else if (sig == SIGSEGV) {
            fprintf(stderr, "Fatal internal error: segmentation violation.\n");
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
#ifdef SIGBUS
        else if (sig == SIGBUS) {
            fprintf(stderr, "Fatal internal error: bus error.\n");
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
#endif
        else if (sig == SIGILL) {
            fprintf(stderr, "Fatal internal error: illegal instruction.\n");
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
        else if (sig == SIGFPE) {
            fprintf(stderr, "Warning: floating point exception.\n");
            return;
        }
    }


    void
    init_signals()
    {
        signal(SIGINT, sig_hdlr);
        signal(SIGSEGV, sig_hdlr);
#ifdef SIGBUS
        signal(SIGBUS, sig_hdlr);
#endif
        signal(SIGILL, sig_hdlr);
        signal(SIGFPE, sig_hdlr);
    }

    void
    exec_startup_script(const char *filename)
    {
        char buf[256];
        strcpy(buf, filename);
        SIfile *sfp = SIfile::create(buf, 0, 0);
        if (!sfp) {
            char *home = pathlist::get_home("XIC_START_DIR");
            if (home) {
                char *t = pathlist::mk_path(home, buf);
                sfp = SIfile::create(t, 0, 0);
                delete [] t;
                delete [] home;
            }
        }
        if (!sfp)
            return;

        SI()->Interpret(sfp, 0, 0, 0);
        delete sfp;
    }
}


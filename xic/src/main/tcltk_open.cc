
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
#include "promptline.h"
#include "errorlog.h"
#include "tcltk_if.h"
#include "tcl_base.h"
#include "reltag.h"

#ifdef HAVE_TCL
#include <dlfcn.h>
#endif


#ifdef HAVE_TCL
namespace {
    // Dynamically open our helper lib, and return a pointer to the
    // interface if successful.
    //
    cTcl_base *find_tcl(bool with_tk, char **lname)
    {
        bool verbose = (getenv("XIC_PLUGIN_DBG") != 0);
        sLstr lstr;
        const char *tclso_path = getenv("XIC_TCLSO_PATH");
        if (tclso_path) {
            // User told us where to look.
            lstr.add(tclso_path);
        }
        else {
            // Look in the plugins directory.
            lstr.add(XM()->ProgramRoot());
            lstr.add("/plugins/");
            lstr.add(with_tk ? "tcltk." : "tcl.");
#ifdef __APPLE__
            lstr.add("dylib");
#else
            lstr.add("so");
#endif
        }

        void *handle = dlopen(lstr.string(), RTLD_LAZY | RTLD_GLOBAL);
        if (!handle) {
            if (verbose)
                printf("dlopen failed: %s\n", dlerror());
            return (0);
        }

        cTcl_base*(*tclptr)() = (cTcl_base*(*)())dlsym(handle, "tclptr");
        if (!tclptr) {
            if (verbose)
                printf("dlsym failed: %s\n", dlerror());
            return (0);
        }

        char idstr[64];
        sprintf(idstr, "%s %s", XM()->OSname(), XIC_RELEASE_TAG);
        cTcl_base *tcl = (*tclptr)();
        if (tcl && (!tcl->id_string() || strcmp(idstr, tcl->id_string()))) {
            if (verbose) {
                printf ("%s plug-in version mismatch:\n"
                    "Xic is \"%s\", plug-in is \"%s\"\n",
                    with_tk ? "Tcl/Tk" : "Tcl", idstr,
                    tcl->id_string() ? tcl->id_string() : "");
            }
            tcl = 0;
        }
        if (!tcl) {
            if (verbose)
                printf("%s interface returned null pointer\n",
                    with_tk ? "Tcl/Tk" : "Tcl");
            return (0);
        }
        if (lname)
            *lname = lstring::copy(lstring::strip_path(lstr.string()));
        return (tcl);
    }


    void bang_tcl(const char *s)
    {
        if (TclIf()->run(s))
            PL()->ShowPrompt("Tcl/Tk done.");
        else {
            Log()->ErrorLog("Tck/Tk Interface", Errs()->get_error());
            PL()->ShowPrompt("Tcl/Tk execution failed.");
        }
    }

}
#endif


cTclIf *
cMain::openTclTk()
{
    cTcl_base *tcl = 0;
#ifdef HAVE_TCL
    char *lname = 0;
    tcl = find_tcl(true, &lname);  // First try with Tk.
    if (!tcl)
        tcl = find_tcl(false, &lname);  // Try again without Tk.
    if (tcl) {
        RegisterBangCmd("tcl", &bang_tcl);
        if (tcl->hasTk()) {
            RegisterBangCmd("tk", &bang_tcl);
            printf("Using Tcl/Tk (%s).\n", lname);
        }
        else
            printf("Using Tcl (%s).\n", lname);
        delete [] lname;
    }
#endif
    return (new cTclIf(tcl));
}


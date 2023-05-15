
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
#include "python_if.h"
#include "reltag.h"

#ifdef HAVE_PYTHON
#include <dlfcn.h>
#endif

#ifdef HAVE_PYTHON
namespace {
    cPy_base *open_py(const char *path)
    {
        bool verbose = (getenv("XIC_PLUGIN_DBG") != 0);
        void *handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
        if (!handle) {
            if (verbose)
                printf("dlopen failed: %s\n", dlerror());
            return (0);
        }

        cPy_base*(*pyptr)() = (cPy_base*(*)())dlsym(handle, "pyptr");
        if (!pyptr) {
            if (verbose)
                printf("dlsym failed: %s\n", dlerror());
            return (0);
        }

        char idstr[64];
        snprintf(idstr, sizeof(idstr), "%s %s", XM()->OSname(),
            XIC_RELEASE_TAG);
        cPy_base *py = (*pyptr)();
        if (py && (!py->id_string() || strcmp(idstr, py->id_string()))) {
            if (verbose) {
                printf ("Python plug-in version mismatch:\n"
                    "Xic is \"%s\", plug-in is \"%s\"\n", idstr,
                    py->id_string() ? py->id_string() : "");
            }
            py = 0;
        }
        if (!py) {
            if (verbose)
                printf ("Python interface returned null pointer\n");
            return (0);
        }
        return (py);
    }


    // Dynamically open our helper lib, and return a pointer to the
    // interface if successful.
    //
    cPy_base *find_py(char **lname)
    {
        char *path = 0;
        const char *pyso_path = getenv("XIC_PYSO_PATH");
        if (pyso_path) {
            // User told us where to look.
            path = lstring::copy(pyso_path);
        }
        else {
            // Look in the plugins directory.
            sLstr lstr;
            lstr.add(XM()->ProgramRoot());
            lstr.add("/plugins/");
            lstr.add("py27.");
#ifdef __APPLE__
            lstr.add("dylib");
#else
            lstr.add("so");
#endif
            path = lstr.string_trim();    // py27.so
        }

        cPy_base *py = open_py(path);
        if (py) {
            // found py27
            if (lname)
                *lname = lstring::copy(lstring::strip_path(path));
            delete [] path;
            return (py);
        }
        char *t = strrchr(path, '.');
        *--t = '6';
        py = open_py(path);
        if (py) {
            // found py26
            if (lname)
                *lname = lstring::copy(lstring::strip_path(path));
            delete [] path;
            return (py);
        }
        t = strrchr(path, '.');
        *--t = '4';
        if (py) {
            // found py24
            if (lname)
                *lname = lstring::copy(lstring::strip_path(path));
            delete [] path;
            return (py);
        }
        delete [] path;
        return (0);
    }


    // The "bang" command to run a Python script.
    //
    void bang_py(const char *s)
    {
        if (PyIf()->run(XM()->Program(), s))
            PL()->ShowPrompt("Python done.");
        else {
            Log()->ErrorLog("Python Interface", Errs()->get_error());
            PL()->ShowPrompt("Python execution failed.");
        }
    }
}
#endif


// Allocate the Python interface.
//
cPyIf *
cMain::openPY()
{
    cPy_base *py = 0;
#ifdef HAVE_PYTHON
    char *lname = 0;
    py = find_py(&lname);
    if (py) {
        RegisterBangCmd("py", &bang_py);
        printf("Using Python (%s).\n", lname);
        delete [] lname;
    }
#endif
    return (new cPyIf(py));
}


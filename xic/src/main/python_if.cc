
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

#include "config.h"
#include "main.h"
#include "promptline.h"
#include "errorlog.h"
#include "dsp_inlines.h"
#include "python_if.h"
#include "reltag.h"

#ifdef HAVE_PYTHON
#include <dlfcn.h>
#endif


//-----------------------------------------------------------------------------
// Interface to Python interpreter.
//

namespace {
    const char *notavail = "Python interface not available.";
}

class cPy_nogo : public cPy_base
{
    const char *id_string() { return (notavail); }
    bool init(SymTab*)      { Errs()->add_error(notavail); return (false); }
    bool run(int, char**)   { Errs()->add_error(notavail); return (false); }
    bool runString(const char*, const char*)
                            { Errs()->add_error(notavail); return (false); }
    bool runModuleFunc(const char*, const char*, Variable*, Variable*)
                            { Errs()->add_error(notavail); return (false); }
    void reset()            { }
    PyObject *wrapper(const char*, PyObject*, int, SIscriptFunc)
                            { return (0); }
};



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
        sprintf(idstr, "%s %s", XM()->OSname(), XIC_RELEASE_TAG);
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
}
#endif


SymTab *cPyIf::py_functions = 0;
cPyIf *cPyIf::instancePtr = 0;

cPyIf::cPyIf()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cPyIf already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

#ifdef HAVE_PYTHON
    char *lname = 0;
    pyPtr = find_py(&lname);
    if (pyPtr) {
        has_py = true;
        XM()->RegisterBangCmd("py", &bang_py);
        printf("Using Python (%s).\n", lname);
        delete [] lname;
        return;
    }
#endif

    pyPtr = new cPy_nogo;
    has_py = false;
}


// Private static error exit.
//
void
cPyIf::on_null_ptr()
{
    fprintf(stderr, "Singleton class cPyIf used before instantiated.\n");
    exit(1);
}


bool
cPyIf::run(const char *s)
{
    bool ret = pyPtr->init(py_functions);
    delete py_functions;
    py_functions = 0;
    if (!ret)
        return (false);

    if (!s)
        s = "";
    stringlist *s0 = 0, *se = 0;
    char *tok;
    while ((tok = lstring::getqtok(&s)) != 0) {
        if (!s0)
            s0 = se = new stringlist(tok, 0);
        else {
            se->next = new stringlist(tok, 0);
            se = se->next;
        }
    }
    int ac = stringlist::length(s0) + 1;
    char **av = new char*[ac+1];
    av[0] = lstring::copy(XM()->Program());
    int i = 1;
    while (s0) {
        av[i++] = s0->string;
        s0->string = 0;
        stringlist *sx = s0;
        s0 = s0->next;
        delete sx;
    }
    av[ac] = 0;

    ret = pyPtr->run(ac, av);
    for (i = 0; i < ac; i++)
        delete [] av[i];
    delete [] av;
    return (ret);
}


bool
cPyIf::runString(const char *prmstr, const char *cmds)
{
    bool ret = pyPtr->init(py_functions);
    delete py_functions;
    py_functions = 0;
    if (!ret)
        return (false);
    return (pyPtr->runString(prmstr, cmds));
}


bool
cPyIf::runModuleFunc(const char *mname, const char *fname, Variable *res,
    Variable *args)
{
    Errs()->init_error();
    if (!mname || !*mname) {
        Errs()->add_error("runModuleFunc: null or empty module name.");
        return (false);
    }
    if (!fname || !*fname) {
        Errs()->add_error("runModuleFunc: null or empty function name.");
        return (false);
    }

    if (py_functions) {
        bool ret = pyPtr->init(py_functions);
        delete py_functions;
        py_functions = 0;
        if (!ret)
            return (false);
    }
    return (pyPtr->runModuleFunc(mname, fname, res, args));
}


void
cPyIf::reset()
{
    pyPtr->reset();
}


// Static function.
// This puts a Python function implementation into the table.  The table
// must be filled before Python is run, at which time the table will be
// loaded into the Python interpreter and destroyed.
//
// The fname MUST be a persistent string!
//
void
cPyIf::register_func(const char *fname, PyObject*(*func)(PyObject*, PyObject*))
{
    if (!fname || !func)
        return;
    if (!py_functions)
        py_functions = new SymTab(false, false);
    SymTabEnt *ent = SymTab::get_ent(py_functions, fname);
    if (!ent)
        py_functions->add(fname, (void*)func, false);
    else
        ent->stData = (void*)func;
}


// Static function.
// The exported "bang" command to run a Python script.
//
void
cPyIf::bang_py(const char *s)
{
    if (PyIf()->run(s))
        PL()->ShowPrompt("Python done.");
    else {
        Log()->ErrorLog("Python Interface", Errs()->get_error());
        PL()->ShowPrompt("Python execution failed.");
    }
}


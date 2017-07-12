
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: py.cc,v 1.25 2017/04/13 17:06:30 stevew Exp $
 *========================================================================*/

#include "config.h"
#undef HAVE_STAT // Clash in Python!

#ifdef HAVE_PYTHON
#include <Python.h>
#include <setjmp.h>
#include "main.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_handle.h"
#include "py_base.h"
#include "reltag.h"

class cPy *Py();

#define Py_ssize_t ssize_t

class cPy : public cPy_base
{
public:
    friend cPy *Py()    { return (cPy::ptr); }

    cPy();
    const char *id_string();
    bool init(SymTab*);
    bool run(int, char**);
    bool runString(const char*, const char*);
    bool runModuleFunc(const char*, const char*, Variable*, Variable*);
    void reset();
    PyObject *wrapper(const char*, PyObject*, int, SIscriptFunc);

    void addErr(const char *err)    { err_lstr.add(err); }
    void exit(int s)                { exit_status = s; longjmp(jbuf, 1); }

private:
    PyObject *wrap_setjmp(PyObject*, PyObject*);
    static PyObject *eval(PyObject*, PyObject*);

    PyMethodDef *methods;
    const char *idstr;
    int level;
    int exit_status;
    jmp_buf jbuf;
    sLstr err_lstr;

    static cPy *ptr;
};


// This is our function to get a cPy through dlsym.
//
extern "C" {
    cPy *pyptr()
    {
        if (Py())
            return (Py());
        return (new cPy);
    }
}

cPy *cPy::ptr = 0;


cPy::cPy()
{
    methods = 0;
    idstr = OSNAME " " XIC_RELEASE_TAG;
    level = 0;
    exit_status = 0;

    ptr = this;
}


#define PROGRAM_NAME "xic"
#define MODULE_NAME "xic"
#define ERRMOD_NAME "xicerr"

// Implement an optional error redirection to the error log.  To use
// this, the Python code must include the lines
// import xicerr
// import sys
// sys.stderr = xicerr
//
namespace {
    PyObject *write(PyObject*, PyObject *args)
    {
        int sz = PyTuple_Size(args);
        sLstr lstr;
        for (int i = 0; i < sz; i++) {
            PyObject *o = PyTuple_GetItem(args, i);
            if (PyString_Check(o)) {
                if (lstr.string())
                    lstr.add_c(' ');
                lstr.add(PyString_AsString(o));
            }
            else {
                PyObject *pstr = PyObject_Str(o);
                if (pstr) {
                    if (lstr.string())
                        lstr.add_c(' ');
                    lstr.add(PyString_AsString(pstr));
                    Py_DECREF(pstr);
                }
                else
                    PyErr_Clear();
            }
        }
        // This function is called multiple times to compose an error
        // message, so it must be buffered before logging.  We will
        // just save the fragments here, and print everything after
        // exiting.
        if (Py())
            Py()->addErr(lstr.string());
        return (Py_None);
    }

    PyMethodDef mdef[] = {
        { (char*)"write", write, METH_VARARGS, 0 },
        { 0, 0, 0, 0 }
    };
}


// Return the id string which is in the form OSNAME CVSTAG.  This must
// match the application.
//
const char *
cPy::id_string()
{
    return (idstr);
}


bool
cPy::init(SymTab *tab)
{
    err_lstr.free();
    Errs()->init_error();

    if (Py_IsInitialized())
        return (true);

    if (!methods) {
        int sz = 2;
        if (tab)
            sz += tab->allocated();
        methods = new PyMethodDef[sz];
        int cnt = 0;
        if (tab) {
            SymTabGen gen(tab);
            SymTabEnt *ent;
            while ((ent = gen.next()) != 0) {
                methods[cnt].ml_name = (char*)ent->stTag;
                methods[cnt].ml_meth = (PyCFunction)ent->stData;
                methods[cnt].ml_flags = METH_VARARGS;
                methods[cnt].ml_doc = (char*)"Xic function";
                cnt++;
            }
        }
        methods[cnt].ml_name = (char*)"eval";
        methods[cnt].ml_meth = eval;
        methods[cnt].ml_flags = METH_VARARGS;
        methods[cnt].ml_doc = (char*)"Xic interface";
        cnt++;
        methods[cnt].ml_name = 0;
        methods[cnt].ml_meth = 0;
        methods[cnt].ml_flags = 0;
        methods[cnt].ml_doc = 0;
    }

    char appname[] = PROGRAM_NAME;
    Py_SetProgramName(appname);
    Py_Initialize();
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
        Errs()->add_error("Py_Initialize generated exception.");
        return (false);
    }
    Py_InitModule((char*)ERRMOD_NAME, mdef);
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
        Errs()->add_error("Py_InitModule generated exception.");
        return (false);
    }
    Py_InitModule((char*)MODULE_NAME, methods);
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
        Errs()->add_error("Py_InitModule generated exception.");
        return (false);
    }
    return (true);
}


bool
cPy::run(int argc, char **argv)
{
    err_lstr.free();
    Errs()->init_error();

    if (!Py_IsInitialized()) {
        Errs()->add_error("Python has not been initialized.");
        return (false);
    }
    int ret = 0;
    if (!level) {
        if (!setjmp(jbuf)) {
            level++;
            SI()->PushLexprCx();
            ret = Py_Main(argc, argv);
            SI()->PopLexprCx();
            level--;
        }
        else {
            reset();
            if (err_lstr.length() > 0) {
                Errs()->add_error(err_lstr.string());
                err_lstr.free();
            }
            if (!Errs()->has_error()) {
                // If no error messages and status-0, return clean.
                if (!exit_status)
                    return (true);

                Errs()->add_error(
                    "Exit with status %d, interpreter reset.", exit_status);
            }
            return (false);
        }
    }
    else {
        level++;
        SI()->PushLexprCx();
        ret = Py_Main(argc, argv);
        SI()->PopLexprCx();
        level--;
    }
    if (err_lstr.length() > 0) {
        Errs()->add_error(err_lstr.string());
        err_lstr.free();
    }
    if (ret == 1)
        Errs()->add_error("Interpreter exited on unhandled exception.");
    else if (ret == 2)
        Errs()->add_error("Interpreter exited on invalid command line.");
    return (!Errs()->has_error());
}


bool
cPy::runString(const char *prmstr, const char *cmds)
{
    err_lstr.free();
    Errs()->init_error();

    if (!Py_IsInitialized()) {
        Errs()->add_error("Python has not been initialized.");
        return (false);
    }

    // Bring in the Xic interface.
    const char *cruft =
        "import xic\nimport xicerr\nimport sys\nsys.stderr=xicerr\n";

    int ret = 0;
    if (!level) {
        if (!setjmp(jbuf)) {
            level++;
            SI()->PushLexprCx();
            ret = PyRun_SimpleString(cruft);
            if (!ret && prmstr)
                ret = PyRun_SimpleString(prmstr);
            if (!ret)
                ret = PyRun_SimpleString(cmds);
            SI()->PopLexprCx();
            level--;
        }
        else {
            reset();
            if (err_lstr.length() > 0) {
                Errs()->add_error(err_lstr.string());
                err_lstr.free();
            }
            if (!Errs()->has_error()) {
                // If no error messages and status-0, return clean.
                if (!exit_status)
                    return (true);

                Errs()->add_error(
                    "Exit with status %d, interpreter reset.", exit_status);
            }
            return (false);
        }
    }
    else {
        level++;
        SI()->PushLexprCx();
        ret = PyRun_SimpleString(cruft);
        if (!ret && prmstr)
            ret = PyRun_SimpleString(prmstr);
        if (!ret)
            ret = PyRun_SimpleString(cmds);
        SI()->PopLexprCx();
        level--;
    }
    if (err_lstr.length() > 0) {
        Errs()->add_error(err_lstr.string());
        err_lstr.free();
    }
    if (ret == 1)
        Errs()->add_error("Interpreter exited on unhandled exception.");
    else if (ret == 2)
        Errs()->add_error("Interpreter exited on invalid command line.");
    return (!Errs()->has_error());
}


namespace {
    // Create a PyObject from the variable.
    //
    const char *var_to_obj(Variable *v, PyObject **pret)
    {
        *pret = 0;
        if (v->type == TYP_STRING || v->type == TYP_NOTYPE) {
            if (!v->content.string)
                // Return int 0 instead
                *pret = PyInt_FromLong(0);
            else
                *pret = PyString_FromString(v->content.string);
            return (0);
        }
        if (v->type == TYP_SCALAR) {
            *pret = PyFloat_FromDouble(v->content.value);
            return (0);
        }
        if (v->type == TYP_ARRAY) {
            int size = v->content.a->length();
            PyObject *list = PyList_New(size);
            for (int i = 0; i < size; i++) {
                PyList_SetItem(list, i,
                    PyFloat_FromDouble(v->content.a->values()[i]));
            }
            *pret = list;
            return (0);
        }
        if (v->type == TYP_ZLIST) {
            Zlist *zl = v->content.zlist;
            int len = Zlist::length(zl);
            PyObject *list = PyList_New(len + 1);
            PyList_SetItem(list, 0, PyString_FromString("zlist"));
            for (int i = 1; i <= len; i++) {
                PyObject *lz = PyList_New(6);
                PyList_SetItem(lz, 0, PyInt_FromLong(zl->Z.xll));
                PyList_SetItem(lz, 1, PyInt_FromLong(zl->Z.xlr));
                PyList_SetItem(lz, 2, PyInt_FromLong(zl->Z.yl));
                PyList_SetItem(lz, 3, PyInt_FromLong(zl->Z.xul));
                PyList_SetItem(lz, 4, PyInt_FromLong(zl->Z.xur));
                PyList_SetItem(lz, 5, PyInt_FromLong(zl->Z.yu));
                PyList_SetItem(list, i, lz);
            }
            *pret = list;
            return (0);
        }
        if (v->type == TYP_HANDLE) {
            // We return any type of handle, however these are useless
            // in Python.  For string lists, however, we include the
            // list of strings.

            int id = mmRnd(v->content.value);
            sHdl *hdl = sHdl::get(id);
            if (hdl) {
                int len = 2;
                if (hdl->type == HDLstring) {
                    stringlist *s0 = (stringlist*)hdl->data;
                    len += stringlist::length(s0);
                }
                PyObject *list = PyList_New(len);
                PyList_SetItem(list, 0, PyString_FromString("xic_handle"));
                PyList_SetItem(list, 1, PyInt_FromLong(id));
                if (hdl->type == HDLstring) {
                    stringlist *s0 = (stringlist*)hdl->data;
                    int cnt = 2;
                    for (stringlist *sl = s0; sl; sl = sl->next) {
                        PyList_SetItem(list, cnt,
                            PyString_FromString(sl->string));
                        cnt++;
                    }
                }
                *pret = list;
            }
            else {
                PyObject *list = PyList_New(2);
                PyList_SetItem(list, 0, PyString_FromString("xic_handle"));
                PyList_SetItem(list, 1, PyInt_FromLong(0));
                *pret = list;
            }
            return (0);
        }
        // TYP_LEXPR
        return ("Unhandled argument type.");
    }


    // Create a tuple from the variables list.
    //
    PyObject *vars_to_tuple(Variable *v, const char **errp)
    {
        *errp = 0;
        int ac = 0;
        for ( ; ; ac++) {
            if (ac > MAXARGC) {
                *errp = "Too many arguments.";
                return (0);
            }
            if (v[ac].type == TYP_ENDARG)
                break;
        }
        PyObject *p = PyTuple_New(ac);
        for (int i = 0; i < ac; i++) {
            PyObject *obj;
            const char *err = var_to_obj(v + i, &obj);
            if (err) {
                Py_DECREF(p);
                *errp = err;
                return (0);
            }
            PyTuple_SetItem(p, i, obj);
        }
        return (p);
    }


    // Set the variable from a PyObject.
    //
    const char *obj_to_var(PyObject *pval, Variable *v)
    {
        if (PyString_Check(pval)) {
            const char *str = PyString_AsString(pval);
            v->type = TYP_STRING;
            v->content.string = lstring::copy(str);
            v->flags |= VF_ORIGINAL;
            return (0);
        }
        if (PyFloat_Check(pval)) {
            double d = PyFloat_AsDouble(pval);
            v->type = TYP_SCALAR;
            v->content.value = d;
            return (0);
        }
        if (PyLong_Check(pval)) {
            long l = PyLong_AsLong(pval);
            v->type = TYP_SCALAR;
            v->content.value = (double)l;
            return (0);
        }
        if (PyBool_Check(pval)) {
            bool b = (pval == Py_True);
            v->type = TYP_SCALAR;
            v->content.value = b ? 1.0 : 0.0;
            return (0);
        }
        if (PyInt_Check(pval)) {
            long l = PyInt_AsLong(pval);
            v->type = TYP_SCALAR;
            v->content.value = (double)l;
            return (0);
        }
        if (PyList_Check(pval)) {
            PyObject *o = PyList_GetItem(pval, 0);
            Py_ssize_t size = PyList_Size(pval);
            if (PyString_Check(o)) {
                const char *str = PyString_AsString(o);
                if (str && !strcmp(str, "zlist")) {
                    // This is a zlist, the remaining elements are
                    // zoids as six-integer lists.
                    Zlist *z0 = 0, *ze = 0;
                    for (Py_ssize_t j = 1; j < size; j++) {
                        o = PyList_GetItem(pval, j);

                        if (!PyList_Check(o) || PyList_Size(o) != 6)
                            return  ("Zlist translation error.");

                        int xll = PyInt_AsLong(PyList_GetItem(o, 0));
                        int xlr = PyInt_AsLong(PyList_GetItem(o, 1));
                        int yl = PyInt_AsLong(PyList_GetItem(o, 2));
                        int xul = PyInt_AsLong(PyList_GetItem(o, 3));
                        int xur = PyInt_AsLong(PyList_GetItem(o, 4));
                        int yu = PyInt_AsLong(PyList_GetItem(o, 5));
                        Zlist *z = new Zlist(xll, xlr, yl, xul, xur, yu, 0);
                        if (!z0)
                            z0 = ze = z;
                        else {
                            ze->next = z;
                            ze = z;
                        }
                    }
                    v->type = TYP_ZLIST;
                    v->content.zlist = z0;
                    return (0);
                }
                if (str && !strcmp(str, "xic_handle")) {
                    o = PyList_GetItem(pval, 1);

                    if (!PyInt_Check(o))
                        return ("Handle translation error.");

                    int id = PyInt_AsLong(o);
                    sHdl *hdl = sHdl::get(id);
                    if (hdl) {
                        v->type = TYP_HANDLE;
                        v->content.value = id;
                    }
                    else {
                        v->type = TYP_SCALAR;
                        v->content.value = 0;
                    }
                    return (0);
                }
                return ("Non-handled list data type in arguments.");
            }

            // Assume array data.
            v->type = TYP_ARRAY;
            v->content.a = new siAryData;
            v->content.a->allocate(size);
            v->content.a->dims()[0] = size;
            double *vals = v->content.a->values();
            v->flags |= VF_ORIGINAL;
            for (Py_ssize_t j = 0; j < size; j++) {
                o = PyList_GetItem(pval, j);
                if (PyFloat_Check(o))
                    vals[j] = PyFloat_AsDouble(o);
                else if (PyLong_Check(o))
                    vals[j] = (double)PyLong_AsLong(o);
                else if (PyBool_Check(o))
                    vals[j] = (o == Py_True) ? 1.0 : 0.0;
                else if (PyInt_Check(o))
                    vals[j] = (double)PyInt_AsLong(o);
                else
                    return ("Non-numeric list data type in array.");
            }
            return (0);
        }
        return ("Non-handled data type.");
    }


    // Fill in the variables from the tuple.
    //
    const char *tuple_to_vars(PyObject *args, Variable *v)
    {
        int ac = PyTuple_Size(args);
        for (int i = 0; i < ac; i++) {
            PyObject *arg = PyTuple_GetItem(args, i);
            const char *err = obj_to_var(arg, v+i);
            if (err)
                return (err);
        }
        v[ac].type = TYP_ENDARG;
        return (0);
    }
}


// Private function.
// Calling this instead of setjmp directly eliminates the GCC warning
// about variables "clobbered by longjmp or vfork".  Explained nicely
// at "http://stackoverflow.com/questions/2024933/"
// "warning-might-be-clobbered-on-c-object-with-setjmp"
//
PyObject *
cPy::wrap_setjmp(PyObject *pfunc, PyObject *pargs)
{
    if (!setjmp(jbuf)) {
        level++;
        PyObject *pval = PyObject_CallObject(pfunc, pargs);
        level--;
        return (pval);
    }
    return (0);
}


bool
cPy::runModuleFunc(const char *modname, const char *funcname, Variable *res,
    Variable *args)
{
    Errs()->init_error();
    PyObject *pmodname = PyString_FromString(modname);
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
        Errs()->add_error("Bad module name %s.", modname);
        return (false);
    }
    PyObject *pmod = PyImport_Import(pmodname);
    Py_DECREF(pmodname);
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
        Errs()->add_error("PyImport_Import generated exception.");
        return (false);
    }

    PyObject *pfunc = PyObject_GetAttrString(pmod, (char*)funcname);
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
        Py_DECREF(pmod);
        Errs()->add_error("PyObject_GetAttrString generated exception.");
        return (false);
    }
    if (!pfunc) {
        Py_DECREF(pmod);
        Errs()->add_error("Function %s not found.", funcname);
        return (false);
    }
    if (!PyCallable_Check(pfunc)) {
        Py_DECREF(pfunc);
        Py_DECREF(pmod);
        Errs()->add_error("Function object not callable.");
        return (false);
    }
    const char *err;
    PyObject *pargs = vars_to_tuple(args, &err);
    if (err) {
        Py_DECREF(pfunc);
        Py_DECREF(pmod);
        Errs()->add_error(err);
        return (false);
    }

    PyObject *pval = 0;
    if (!level) {
        SI()->PushLexprCx();
        pval = wrap_setjmp(pfunc, pargs);
        SI()->PopLexprCx();
        if (!pval) {
            reset();
            if (!Errs()->has_error()) {
                // If no error messages and status-0, return clean.
                if (!exit_status)
                    return (true);

                Errs()->add_error(
                    "Exit with status %d, interpreter reset.", exit_status);
            }
            return (false);
        }
    }
    else {
        level++;
        SI()->PushLexprCx();
        pval = PyObject_CallObject(pfunc, pargs);
        SI()->PopLexprCx();
        level--;
    }
    Py_DECREF(pargs);
    Py_DECREF(pfunc);
    Py_DECREF(pmod);
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
        Errs()->add_error("PyObject_CallObject generated exception.");
        return (false);
    }
    err = obj_to_var(pval, res);
    Py_DECREF(pval);
    if (err) {
        Errs()->add_error(err);
        return (false);
    }
    return (true);
}


void
cPy::reset()
{
    err_lstr.free();

    if (Py_IsInitialized()) {
        Py_Finalize();

        // Py_Finalize resets the SIGINT handler (at least) to exit,
        // so reset signal handling.
        XM()->InitSignals(true);
    }
    level = 0;
    exit_status = 0;
}


namespace {
    // Take care of array returns.  Anything else is assumed to be
    // passed as a copy.
    //
    void update_array_vals(PyObject *tpl, Variable *v)
    {
        int ac = PyTuple_Size(tpl);
        for (int i = 0; i < ac; i++) {
            if (v[i].type == TYP_ARRAY) {
                PyObject *arg = PyTuple_GetItem(tpl, i);
                int size = v[i].content.a->length();
                for (int j = 0; j < size; j++) {
                    PyList_SetItem(arg, j,
                        PyFloat_FromDouble(v[i].content.a->values()[j]));
                }
            }
        }
    }


    void clear_args(Variable *v, int ac)
    {
        for (int i = 0; i < ac; i++)
            v[i].clear();
    }
}


// A wrapper for internal Xic functions.  This allows internal functions
// to be called using xic.funcname(...).
//
PyObject *
cPy::wrapper(const char *fname, PyObject *args, int nargs,
    SIscriptFunc xicfunc)
{
    char buf[256];
    if (!fname || !xicfunc) {
        PyErr_SetString(PyExc_RuntimeError, "Null pointer passed to wrapper.");
        return (Py_None);
    }
    int ac = args ? PyTuple_Size(args) : 0;
    if (nargs != VARARGS && ac != nargs) {
        snprintf(buf, 256, "Wrong argument count %d to %s, requires %d.",
            ac, fname, nargs);
        PyErr_SetString(PyExc_RuntimeError, buf);
        return (Py_None);
    }
    if (nargs == VARARGS && ac > MAXARGC) {
        snprintf(buf, 256, "Too many args to %s, max %d.", fname,
            MAXARGC);
        PyErr_SetString(PyExc_RuntimeError, buf);
        return (Py_None);
    }
    Variable v[MAXARGC + 1];
    const char *err = tuple_to_vars(args, v);
    if (err) {
        snprintf(buf, 256, "Converting arg list for %s: %s", fname, err);
        PyErr_SetString(PyExc_RuntimeError, buf);
        clear_args(v, ac);
        return (Py_None);
    }
    Variable res;
    if ((*xicfunc)(&res, v, SI()->LexprCx()) != OK) {
        snprintf(buf, 256, "Execution of %s failed.", fname);
        PyErr_SetString(PyExc_RuntimeError, buf);
        clear_args(v, ac);
        return (Py_None);
    }
    update_array_vals(args, v);
    PyObject *oret;
    err = var_to_obj(&res, &oret);
    if (err) {
        snprintf(buf, 256, "Execution of %s returned unhandled type.",
            fname);
        PyErr_SetString(PyExc_RuntimeError, buf);
        clear_args(v, ac);
        return (Py_None);
    }
    clear_args(v, ac);
    res.clear();
    return (oret);
}


// Static function.
// A method that allows both internal and user-defined functions to
// be called using the form xic.eval("funcname", ...).  This is the
// only way to execute user-defined functions.
//
PyObject *
cPy::eval(PyObject*, PyObject *args)
{
    int ac = PyTuple_Size(args);
    if (ac < 1) {
        PyErr_SetString(PyExc_RuntimeError,
            "First argument is not Xic function name.");
        return (0);
    }

    PyObject *funcname = PyTuple_GetItem(args, 0);
    if (!PyString_Check(funcname)) {
        PyErr_SetString(PyExc_RuntimeError,
            "First argument is not Xic function name.");
        return (0);
    }

    char buf[256];
    ac--;
    const char *fname = PyString_AsString(funcname);

    // First check the user-defined function table.
    int argc;
    SIfunc *sf = SI()->GetSubfunc(fname, &argc);
    if (sf) {
        if (argc != VARARGS && argc != ac) {
            snprintf(buf, 256, "Wrong arg count %d to %s, requires %d.",
                ac, fname, argc);
            PyErr_SetString(PyExc_RuntimeError, buf);
            return (0);
        }
        if (argc == VARARGS && ac >= MAXARGC) {
            snprintf(buf, 256, "Too many args to %s, max %d.", fname,
                MAXARGC);
            PyErr_SetString(PyExc_RuntimeError, buf);
            return (0);
        }
        siVariable v[MAXARGC + 1];
        const char *err = tuple_to_vars(args, v);
        if (err) {
            snprintf(buf, 256, "Converting arg list for %s: %s", fname, err);
            PyErr_SetString(PyExc_RuntimeError, buf);
            clear_args(v, ac);
            return (0);
        }
        siVariable res;
        if (SI()->EvalFunc(sf, SI()->LexprCx(), v+1, &res) != XIok) {
            snprintf(buf, 256, "Execution of %s failed.", fname);
            PyErr_SetString(PyExc_RuntimeError, buf);
            clear_args(v, ac);
            return (0);
        }
        update_array_vals(args, v+1);
        PyObject *oret;
        err = var_to_obj(&res, &oret);
        if (err) {
            snprintf(buf, 256, "Execution of %s returned unhandled type.",
                fname);
            PyErr_SetString(PyExc_RuntimeError, buf);
            clear_args(v, ac);
            return (0);
        }
        clear_args(v, ac);
        res.clear();
        return (oret);
    }

    // Try the internal functions, it is more efficient to use the
    // wrapper for this.
    SIptfunc *ptf = SIparse()->function(fname);
    if (ptf) {
        if (ptf->argc() != VARARGS && ptf->argc() != ac) {
            snprintf(buf, 256, "Wrong arg count %d to %s, requires %d.",
                ac, fname, ptf->argc());
            PyErr_SetString(PyExc_RuntimeError, buf);
            return (0);
        }
        if (ptf->argc() == VARARGS && ac >= MAXARGC) {
            snprintf(buf, 256, "Too many args to %s, max %d.", fname,
                MAXARGC);
            PyErr_SetString(PyExc_RuntimeError, buf);
            return (0);
        }
        Variable v[MAXARGC + 1];
        const char *err = tuple_to_vars(args, v);
        if (err) {
            snprintf(buf, 256, "Converting arg list for %s: %s", fname, err);
            PyErr_SetString(PyExc_RuntimeError, buf);
            clear_args(v, ac);
            return (0);
        }
        Variable res;
        if ((*ptf->func())(&res, v+1, SI()->LexprCx()) != OK) {
            snprintf(buf, 256, "Execution of %s failed.", fname);
            PyErr_SetString(PyExc_RuntimeError, buf);
            clear_args(v, ac);
            return (0);
        }
        update_array_vals(args, v+1);
        PyObject *oret;
        err = var_to_obj(&res, &oret);
        if (err) {
            snprintf(buf, 256, "Execution of %s returned unhandled type.",
                fname);
            PyErr_SetString(PyExc_RuntimeError, buf);
            clear_args(v, ac);
            return (0);
        }
        clear_args(v, ac);
        res.clear();
        return (oret);
    }
    snprintf(buf, 256, "Function not found: %s.", fname);
    PyErr_SetString(PyExc_RuntimeError, buf);
    return (0);
}


// C overrides for Python to avoid exiting the process.
//
extern "C" {
    void Py_Exit(int status)
    {
        Py()->exit(status);
    }

    void Py_FatalError(const char *message)
    {
        Errs()->add_error(message);
        Py()->exit(1);
    }
}

#endif  // HAVE_PYTHON


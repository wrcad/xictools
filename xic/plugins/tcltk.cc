
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
 $Id: tcltk.cc,v 1.20 2017/04/13 17:06:30 stevew Exp $
 *========================================================================*/

#include "config.h"

#ifdef NO_TK
#undef HAVE_TK
#endif

#ifdef HAVE_TCL
#include <tcl.h>
#ifdef HAVE_TK
#include <tk.h>
#endif
#include <setjmp.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "main.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "tcl_base.h"
#include "reltag.h"


class cTclTk *TclTk();

typedef void(*fTcl_DeleteInterp)(Tcl_Interp*);

class cTclTk : public cTcl_base
{
public:
    friend cTclTk *TclTk()  { return (cTclTk::ptr); }
    friend void Tcl_DeleteInterp(Tcl_Interp*);
    friend int Tcl_AppInit(Tcl_Interp*);
    friend void Tcl_Exit(int);
#ifdef HAVE_TK
    friend void Tk_MainLoop();
#endif

    cTclTk();

    const char *id_string();
    bool init(SymTab*);
    bool run(const char*, bool);
    bool runString(const char*, const char*);
    void reset();
    int wrapper(const char*, Tcl_Interp*, Tcl_Obj* const*, int, int,
        SIscriptFunc);

    bool hasTk()    { return (has_tk); }

private:
    void tcl_DeleteInterp(Tcl_Interp *interp)
        {
            if (!in_main)
                (*tcl_delete_interp)(interp);
            else
                in_main = false;
        }

    void tcl_Exit(int status)
        {
            if (status != 0) {
                // uh-oh, fatal error
                longjmp(jbuf, status);
            }
        }

    void tk_MainLoop()
        {
            in_main = true;
        }


    static int app_init(Tcl_Interp*);

    static int exit_proc(ClientData, Tcl_Interp*, int, char**);
    static int xic_proc(ClientData, Tcl_Interp*, int, char**);
    static int xwin_proc(ClientData, Tcl_Interp*, int, char**);
    static void run_tcltk(const char*);

    Tcl_Interp *interp_exiting;
    SymTab *functions;
    const char *idstr;
    bool in_main;
    bool has_tk;
    jmp_buf jbuf;

    static fTcl_DeleteInterp tcl_delete_interp;
    static cTclTk *ptr;
};


namespace {
    // Calling this instead of setjmp directly eliminates the GCC
    // warning about variables "clobbered by longjmp or vfork".
    // Explained nicely at
    // "http://stackoverflow.com/questions/2024933/"
    // "warning-might-be-clobbered-on-c-object-with-setjmp"
    //
    inline int wrap_setjmp(jmp_buf bf)
    {
        return (setjmp(bf));
    }
}


extern "C" {
    // This is our function to get a cTclTk through dlsym.
    //
    cTclTk *tclptr()
    {
        if (TclTk())
            return (TclTk());
        return (new cTclTk);
    }

    // Override functions for Tcl/Tk.

    // The Tk_MainEx() function that is used as the main function for
    // running a script ends with the following three lines:
    //    Tk_MainLoop();
    //    Tcl_DeleteInterp(interp);
    //    Tcl_Exit(0);
    // A more convenient architecture would separate the
    // initialization and main loop functions, but such is life.
    // Below, we override these functions, and handle the loop
    // ourselves.


    // This function should work normally, except if called just after
    // Tk_MainLoop().
    //
    void Tcl_DeleteInterp(Tcl_Interp *interp)
    {
        TclTk()->tcl_DeleteInterp(interp);
    }


    // The initialization callback.
    //
    int Tcl_AppInit(Tcl_Interp *interp)
    {
        return (TclTk()->app_init(interp));
    }


    // We *never* want to exit the process, so this is a no-op.  The
    // exit command is redefined below.
    //
    void Tcl_Exit(int status)
    {
        TclTk()->tcl_Exit(status);
    }


#ifdef HAVE_TK
    // This does nothing except set a flag for Tcl_DeleteInterp().
    //
    void Tk_MainLoop()
    {
        TclTk()->tk_MainLoop();
    }
#endif
} // extern "C"


fTcl_DeleteInterp cTclTk::tcl_delete_interp = 0;
cTclTk *cTclTk::ptr = 0;

cTclTk::cTclTk()
{
    interp_exiting = 0;
    functions = 0;
    idstr = OSNAME" "CVS_RELEASE_TAG;
    in_main = false;
#ifdef HAVE_TK
    has_tk = true;
#else
    has_tk = false;
#endif
    ptr = this;
}


// Return the id string which is in the form OSNAME CVSTAG.  This must
// match the application.
//
const char *
cTclTk::id_string()
{
    return (idstr);
}


// Initialization is trivial, just hand off the functions table and
// set the app executable name.
//
bool
cTclTk::init(SymTab *tab)
{
    if (!functions)
        functions = tab;
    Tcl_FindExecutable(XM()->Program());
    return (true);
}


namespace {
    char **mk_args(const char *s, int *nargs)
    {
        *nargs = 0;
        stringlist *s0 = new stringlist(lstring::copy(XM()->Program()), 0);
        stringlist *se = s0;
        int cnt = 1;
        char *tok;
        while ((tok = lstring::getqtok(&s)) != 0) {
            se->next = new stringlist(tok, 0);
            se = se->next;
            cnt++;
        }
        char **a = new char*[cnt+1];
        cnt = 0;
        while (s0) {
            a[cnt] = s0->string;
            s0->string = 0;
            stringlist *sx = s0;
            s0 = s0->next;
            delete sx;
            cnt++;
        }
        a[cnt] = 0;
        *nargs = cnt;
        return (a);
    }


    void free_args(char **a)
    {
        for (int i = 0; a[i]; i++)
            delete [] a[i];
        delete [] a;
    }
}


// Public function to run a tcl/tk script.  The argument contains
// the script name plus any arguments.  For tk, this blocks, while
// servicing events, until the last window is destroyed.  Tor tcl, the
// script executes and returns.
//
bool
cTclTk::run(const char *s, bool is_tk)
{
    Errs()->init_error();
    int argc;
    char **argv = mk_args(s, &argc);

    if (!wrap_setjmp(jbuf)) {
        if (!is_tk) {
            SI()->PushLexprCx();
            Tcl_Main(argc, argv, app_init);
            SI()->PopLexprCx();
        }
        else {
#ifdef HAVE_TK
            SI()->PushLexprCx();
            Tcl_Interp *interp = Tcl_CreateInterp();
            if (!wrap_setjmp(jbuf)) {
                Tk_MainEx(argc, argv, app_init, interp);

                while (Tk_GetNumMainWindows() > 0) {
                    if (wrap_setjmp(jbuf))
                        break;
                    while (Tcl_DoOneEvent(TCL_DONT_WAIT)) ;
                    if (interp_exiting == interp) {
                        interp_exiting = 0;
                        break;
                    }
                    if (SIparse()->ifCheckInterrupt())
                        break;
                }
            }
            Tcl_DeleteInterp(interp);
            SI()->PopLexprCx();
#else
            free_args(argv);
            Errs()->add_error("Tk is not available.");
            return (false);
#endif
        }
    }
    free_args(argv);
    return (!Errs()->has_error());
}


// Run the cmds as a Tcl script.
//
bool
cTclTk::runString(const char *prmstr, const char *cmds)
{
    Errs()->init_error();
    if (!wrap_setjmp(jbuf)) {
        SI()->PushLexprCx();
        Tcl_Interp *interp = Tcl_CreateInterp();
        Tcl_AppInit(interp);
        int ret = TCL_OK;
        if (prmstr)
            ret = Tcl_Eval(interp, prmstr);
        if (ret == TCL_OK)
            ret = Tcl_Eval(interp, cmds);
        if (ret == TCL_ERROR)
            Errs()->add_error(Tcl_GetStringResult(interp));
        Tcl_DeleteInterp(interp);
        SI()->PopLexprCx();
    }
    return (!Errs()->has_error());
}


void
cTclTk::reset()
{
}


// This prefixes handle values, which aren't scalars anymore.
#define HANDLE_TOK "<xic-handle>"

namespace {
    // Arrays are passed as string arguments in the form "&arrayname()".
    // This returns true if the name token names an array, and returns the
    // size and values.  The subscripts are required to be "0", "1", etc.
    //
    bool is_array(Tcl_Interp *interp, const char *name, int *size,
        double **vals, char **newname)
    {
        *size = 0;
        *vals = 0;
        if (*name != '&')
            return (false);
        name++;
        if (*(name + strlen(name) - 1) != ')')
            return (false);
        const char *t = strchr(name, '(');
        if (!t)
            return (false);
        char buf[64];
        strncpy(buf, name, t - name);
        buf[t-name] = 0;
        char nbuf[32];
        for (int i = 0; ; i++) {
            mmItoA(nbuf, i);
            const char *s = Tcl_GetVar2(interp, buf, nbuf, 0);
            if (!s) {
                *size = i;
                break;
            }
        }
        if (*size == 0)
            return (false);
        *vals = new double[*size];
        for (int i = 0; ; i++) {
            mmItoA(nbuf, i);
            const char *s = Tcl_GetVar2(interp, buf, nbuf, 0);
            if (!s)
                break;
            (*vals)[i] = atof(s);
        }
        *newname = lstring::copy(buf);
        return (true);
    }


    // Set up the argument variables array from the argc/argv supplied.
    // The variable type is determined here.
    //
    void args_to_var(Tcl_Interp *interp, int ac, char **av, Variable *v)
    {
        for (int i = 0; i < ac; i++) {
            const char *s = av[i];
            bool err;
            double d = SIparse()->numberParse(&s, &err);
            if (!err && !*s) {
                v[i].type = TYP_SCALAR;
                v[i].content.value = d;
            }
            else {
                int size;
                double *vals;
                char *name;
                if (is_array(interp, av[i], &size, &vals, &name)) {
                    v[i].type = TYP_ARRAY;
                    v[i].name = name;
                    v[i].content.a = new siAryData;
                    v[i].content.a->allocate(size);
                    v[i].content.a->dims()[0] = size;
                    memcpy(v[i].content.a->values(), vals, size*sizeof(double));
                    delete [] vals;
                    v[i].flags |= VF_ORIGINAL;
                }
                else if (lstring::prefix(HANDLE_TOK, av[i])) {
                    v[i].type = TYP_HANDLE;
                    v[i].content.value = atoi(av[i] + strlen(HANDLE_TOK));
                }
                else {
                    v[i].type = TYP_STRING;
                    char *stmp = av[i];
                    if (*stmp == '"')
                        stmp++;
                    stmp = lstring::copy(stmp);
                    v[i].content.string = stmp;
                    if (*av[i] == '"') {
                        stmp += strlen(stmp) - 1;
                        if (*stmp == '"')
                            *stmp = 0;
                    }
                    v[i].flags |= VF_ORIGINAL;
                }
            }
        }
        v[ac].type = TYP_ENDARG;
    }


    void args_to_var(Tcl_Interp *interp, int ac, Tcl_Obj *const *argv,
        Variable *v)
    {
        for (int i = 0; i < ac; i++) {
            const char *s = Tcl_GetString(argv[i]);
            bool err;
            double d = SIparse()->numberParse(&s, &err);
            if (!err && !*s) {
                v[i].type = TYP_SCALAR;
                v[i].content.value = d;
            }
            else {
                int size;
                double *vals;
                char *name;
                if (is_array(interp, s, &size, &vals, &name)) {
                    v[i].type = TYP_ARRAY;
                    v[i].name = name;
                    v[i].content.a = new siAryData;
                    v[i].content.a->allocate(size);
                    v[i].content.a->dims()[0] = size;
                    memcpy(v[i].content.a->values(), vals, size*sizeof(double));
                    delete [] vals;
                    v[i].flags |= VF_ORIGINAL;
                }
                else if (lstring::prefix(HANDLE_TOK, s)) {
                    v[i].type = TYP_HANDLE;
                    v[i].content.value = atoi(s + strlen(HANDLE_TOK));
                }
                else {
                    v[i].type = TYP_STRING;
                    v[i].content.string = lstring::copy(*s == '"' ? s+1 : s);
                    if (*s == '"') {
                        char *stmp = v[i].content.string;
                        stmp += strlen(stmp) - 1;
                        if (*stmp == '"')
                            *stmp = 0;
                    }
                    v[i].flags |= VF_ORIGINAL;
                }
            }
        }
        v[ac].type = TYP_ENDARG;
    }


    // Process the return after a function call.  This takes care of
    // updating array returns.
    //
    int proc_return(Tcl_Interp *interp, const char *funcname, int ac,
        Variable *v, Variable *res)
    {
        // Take care of array returns.  Anything else is assumed to be
        // passed as a copy
        char buf[256], nbuf[64];
        for (int i = 0; i < ac; i++) {
            if (v[i].type == TYP_ARRAY) {
                int size = v[i].content.a->length();
                for (int j = 0; j < size; j++) {
                    mmItoA(nbuf, j);
                    mmDtoA(buf, v[i].content.a->values()[j], 9, true);
                    Tcl_SetVar2(interp, v[i].name, nbuf, buf, 0);
                }
            }
        }

        // Now set the tcl variable from the return.
        if (res->type == TYP_NOTYPE || res->type == TYP_STRING) {
            const char *t = res->content.string;
            if (!t)
                t = "";
            Tcl_Obj *o = Tcl_NewStringObj(t, -1);
            Tcl_SetObjResult(interp, o);
        }
        else if (res->type == TYP_SCALAR) {
            Tcl_Obj *o = Tcl_NewDoubleObj(res->content.value);
            Tcl_SetObjResult(interp, o);
        }
        else if (res->type == TYP_ARRAY) {
            snprintf(buf, 256,  "execution of %s returned array, not supported",
                funcname);
            Tcl_AddErrorInfo(interp, buf);
            return (TCL_ERROR);
        }
        else if (res->type == TYP_ZLIST) {
            snprintf(buf, 256, "execution of %s returned zlist, not supported",
                funcname);
            Tcl_AddErrorInfo(interp, buf);
            return (TCL_ERROR);
        }
        else if (res->type == TYP_LEXPR) {
            snprintf(buf, 256,  "execution of %s returned lexpr, not supported",
                funcname);
            Tcl_AddErrorInfo(interp, buf);
            return (TCL_ERROR);
        }
        else if (res->type == TYP_HANDLE) {
            if ((int)res->content.value == 0) {
                Tcl_Obj *o = Tcl_NewIntObj(0);
                Tcl_SetObjResult(interp, o);
            }
            else {
                int d = mmRnd(res->content.value);
                snprintf(buf, 256, "%s%d", HANDLE_TOK, d);
                Tcl_SetResult(interp, buf, TCL_VOLATILE);
            }
        }
        else {
            snprintf(buf, 256, "execution of %s returned unknown type",
                funcname);
            Tcl_AddErrorInfo(interp, buf);
            return (TCL_ERROR);
        }
        return (TCL_OK);
    }
}


int
cTclTk::wrapper(const char *fname, Tcl_Interp *interp, Tcl_Obj* const *args,
    int argc, int numargs, SIscriptFunc xicfunc)
{
    char buf[256];
    if (numargs != VARARGS && numargs != argc-1) {
        snprintf(buf, 256, "wrong arg count to %s, requires %d",
            fname, numargs);
        Tcl_AddErrorInfo(interp, buf);
        return (TCL_ERROR);
    }
    if (numargs == VARARGS && argc-1 > MAXARGC) {
        snprintf(buf, 256, "too many args to %s, max %d", fname,
            MAXARGC);
        Tcl_AddErrorInfo(interp, buf);
        return (TCL_ERROR);
    }
    Variable v[MAXARGC + 1];
    args_to_var(interp, argc-1, args+1, v);
    Variable res;
    int ret;
    if ((*xicfunc)(&res, v, SI()->LexprCx()) != OK) {
        snprintf(buf, 256, "execution of %s failed", fname);
        Tcl_AddErrorInfo(interp, buf);
        ret = TCL_ERROR;
    }
    else
        ret = proc_return(interp, fname, argc-1, v, &res);
    for (int i = 0; i < argc; i++)
        v[i].clear();
    res.clear();
    return (ret);
}


// Static function.
// This is the interface to Xic, implementing a Tcl command
//   xic func_name func_args...
// The func_name can be a script interface function or user defined
// function.
//
int
cTclTk::xic_proc(ClientData, Tcl_Interp *interp, int argc, char **argv)
{
    char buf[256];
    if (argc == 1) {
        Tcl_AddErrorInfo(interp, "no function name given");
        return (TCL_ERROR);
    }
    int ac;
    SIfunc *sf = SI()->GetSubfunc(argv[1], &ac);
    if (sf) {
        if (ac != VARARGS && ac != argc - 2) {
            snprintf(buf, 256, "wrong arg count to %s, requires %d",
                argv[1], ac);
            Tcl_AddErrorInfo(interp, buf);
            return (TCL_ERROR);
        }
        if (ac == VARARGS && argc - 2 >= MAXARGC) {
            snprintf(buf, 256, "too many args to %s, max %d", argv[1],
                MAXARGC);
            Tcl_AddErrorInfo(interp, buf);
            return (TCL_ERROR);
        }
        siVariable args[MAXARGC + 1];
        args_to_var(interp, argc-2, argv+2, args);
        siVariable res;
        int ret;
        if (SI()->EvalFunc(sf, SI()->LexprCx(), args, &res) != XIok) {
            snprintf(buf, 256, "execution of %s failed", argv[1]);
            Tcl_AddErrorInfo(interp, buf);
            ret = TCL_ERROR;
        }
        else
            ret = proc_return(interp, argv[1], argc-2, args, &res);
        for (int i = 0; i < argc-2; i++)
            args[i].clear();
        res.clear();
        return (ret);
    }
    SIptfunc *ptf = SIparse()->function(argv[1]);
    if (ptf) {
        if (ptf->argc() != VARARGS && ptf->argc() != argc - 2) {
            snprintf(buf, 256, "wrong arg count to %s, requires %d",
                argv[1], ptf->argc());
            Tcl_AddErrorInfo(interp, buf);
            return (TCL_ERROR);
        }
        if (ptf->argc() == VARARGS && argc - 2 >= MAXARGC) {
            snprintf(buf, 256, "too many args to %s, max %d", argv[1],
                MAXARGC);
            Tcl_AddErrorInfo(interp, buf);
            return (TCL_ERROR);
        }
        Variable args[MAXARGC + 1];
        args_to_var(interp, argc-2, argv+2, args);
        Variable res;
        int ret;
        if ((*ptf->func())(&res, args, SI()->LexprCx()) != OK) {
            snprintf(buf, 256, "execution of %s failed", argv[1]);
            Tcl_AddErrorInfo(interp, buf);
            ret = TCL_ERROR;
        }
        else
            ret = proc_return(interp, argv[1], argc-2, args, &res);
        for (int i = 0; i < argc-2; i++)
            args[i].clear();
        res.clear();
        return (ret);
    }
    snprintf(buf, 256, "unknown function %s", argv[1]);
    Tcl_AddErrorInfo(interp, buf);
    return (TCL_ERROR);
}


#ifdef HAVE_TK
// Static function.
// Tcl command to return the x window id given a widget path.
//
int
cTclTk::xwin_proc(ClientData, Tcl_Interp *interp, int argc, char **argv)
{
    char buf[256];
    if (argc > 2) {
        snprintf(buf, 256, "too many args to %s", argv[0]);
        Tcl_AddErrorInfo(interp, buf);
        return (TCL_ERROR);
    }
    if (argc < 2) {
        snprintf(buf, 256, "too few args to %s", argv[0]);
        Tcl_AddErrorInfo(interp, buf);
        return (TCL_ERROR);
    }
    Tk_Window main = Tk_MainWindow(interp);
    if (!main) {
        Tcl_AddErrorInfo(interp, "no main window");
        return (TCL_ERROR);
    }
    Tk_Window win = Tk_NameToWindow(interp, argv[1], main);
    if (!win) {
        snprintf(buf, 256, "can't find %s", argv[1]);
        Tcl_AddErrorInfo(interp, buf);
        return (TCL_ERROR);
    }
    Tcl_Obj *o = Tcl_NewLongObj(Tk_WindowId(win));
    Tcl_SetObjResult(interp, o);
    return (TCL_OK);
}
#endif
// End of cTclTc functions.


// Static Function.
//
int
cTclTk::app_init(Tcl_Interp *interp)
{
    if (!tcl_delete_interp) {
        tcl_delete_interp = (fTcl_DeleteInterp)dlsym(RTLD_NEXT,
            "Tcl_DeleteInterp");
        if (!tcl_delete_interp) {
            fprintf(stderr, "%s\n", dlerror());
            return (TCL_ERROR);
        }
    }
    if (Tcl_Init(interp) == TCL_ERROR)
        return (TCL_ERROR);

    // Call the init procedures for included packages.  Each call should
    // look like this:
    //
    // if (Mod_Init(interp) == TCL_ERROR) {
    //     return TCL_ERROR;
    // }
    //
    // where "Mod" is the name of the module.
    //
#ifdef HAVE_TK
    if (Tk_Init(interp) == TCL_ERROR)
        return (TCL_ERROR);
    Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);
#endif

    // Call tcl_CreateCommand for application-specific commands, if
    // they weren't already created by the init procedures called above.

    Tcl_CreateCommand(interp, "exit", (Tcl_CmdProc*)exit_proc, 0, 0);
    Tcl_CreateCommand(interp, "xic", (Tcl_CmdProc*)xic_proc, 0, 0);
#ifdef HAVE_TK
    Tcl_CreateCommand(interp, "xwin", (Tcl_CmdProc*)xwin_proc, 0, 0);
#endif

    // The Xic imports are "object commands" that take Tcl_Obj
    // arguments instead of old-fashioned char* arguments.  This
    // executes faster.
    //
    SymTabGen gen(TclTk()->functions);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0)
        Tcl_CreateObjCommand(interp, ent->stTag, (Tcl_ObjCmdProc*)ent->stData,
            0, 0);

    // Specify a user-specific startup file to invoke if the
    // application is run interactively.  Typically the startup
    // file is "~/.apprc" where "app" is the name of the
    // application.  If this line is deleted then no user-specific
    // startup file will be run under any conditions.

    Tcl_SetVar(interp, "Tcl_rcFileName", "~/.xic-wishrc", TCL_GLOBAL_ONLY);
    return (TCL_OK);
}


// Static function.
// This overrides the exit command.  It exports the interpreter so
// we know below which loop to break out of.
//
int
cTclTk::exit_proc(ClientData, Tcl_Interp *interp, int, char**)
{
    TclTk()->interp_exiting = interp;
    return (TCL_OK);
}

#endif  // HAVE_TCL


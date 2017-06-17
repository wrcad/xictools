
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
 $Id: cd_variable.h,v 5.41 2015/03/04 05:15:49 stevew Exp $
 *========================================================================*/

#ifndef CD_VARIABLE_H
#define CD_VARIABLE_H

#include <ctype.h>

// Application-supplied callback function registered with
// VarDB::register_internal.  Called whenever the variable is set or
// cleared.  Arguments are the variable name and set/clear status to
// be applied.  If false is returned during set, the set is aborted.
//
typedef bool(*CDvarProc)(const char*, bool);

// Application-supplied hook checked when a variable is accessed.  The
// argument is the variable name.  If non-null is returned, that
// string rather than a string from the database is returned by the
// access function.
//
typedef const char*(*CDvarGetHookProc)(const char*);

// Application-supplied hook checked when a variable is set. 
// Arguments are the variable name and string.  If true is returned,
// no set operation takes place.
//
typedef bool(*CDvarSetHookProc)(const char*, const char*);

// Application-supplied function to be called once only after a set or
// clear operation completes.  Set with VarDB::register_postfunc in
// the CDvarProc.  The argument is the updated string for set, null
// for clear.  Not called if a hook precedure intervenes, or the
// CDvarProc returns false.
//
typedef void(*CDvarPostProc)(const char*);

// The application should set up the known variables and hooks.


// List element to hold variable state, used to push/restore.
//
struct sVbak
{
    sVbak(const char*, sVbak*);
    ~sVbak();

    const char *vb_name;    // variable name
    char *vb_value;         // copy of initial value if any
    CDvarProc vb_func;      // initial handler if any
    sVbak *next;
};


typedef void CDvarTab;

inline class cCDvdb *CDvdb();

// Class for maintaining the variable database.
//
class cCDvdb
{
    // Table element for a variable that has been set.
    //
    struct var_t
    {
        var_t(char *n, char *s)
            {
                name = n;
                string = s;
                next = 0;
            }

        ~var_t()
            {
                delete [] name;
                delete [] string;
            }

        const char *tab_name()    const { return (name); }
        var_t *tab_next()         const { return (next); }
        void set_tab_next(var_t *t)     { next = t; }
        var_t *tgen_next(bool)    const { return (next); }

        char *string;
    private:
        char *name;
        var_t *next;
    };

    // Table element for application-significant variables.
    //
    template <class T>
    struct intvar_t
    {
        intvar_t(char *n, T c)
            {
                name = n;
                cb = c;
                next = 0;
            }

        ~intvar_t() { delete [] name; }

        const char *tab_name()          { return (name); }
        intvar_t *tab_next()            { return (next); }
        void set_tab_next(intvar_t *t)  { next = t; }
        intvar_t *tgen_next(bool)       { return (next); }

        void set_tab_name(const char *n) { name = n; }

        T cb;
    private:
        const char *name;
        intvar_t *next;
    };

    static cCDvdb *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cCDvdb *CDvdb() { return (cCDvdb::ptr()); }

    cCDvdb();
    ~cCDvdb();

    // The global hooks are for cases where the name recognition
    // logically uses wildcarding, i.e., is not appropriate for a hash
    // table.

    // Register to callback called whenever a variable is accessed.
    //
    CDvarGetHookProc registerGlobalGetHook(CDvarGetHookProc f)
        {
            CDvarGetHookProc r = v_global_get_hook;
            v_global_get_hook = f;
            return (r);
        }

    // Register to callback called whenever a variable is set.
    //
    CDvarSetHookProc registerGlobalSetHook(CDvarSetHookProc f)
        {
            CDvarSetHookProc r = v_global_set_hook;
            v_global_set_hook = f;
            return (r);
        }

    // Register a function to call after set/clear, this will be
    // zeroed after use.  This may be called from the evaluation
    // function in the table.
    //
    void registerPostFunc(CDvarPostProc cb)
        {
            v_post_func = cb;
        }

    bool setVariable(const char*, const char*);
    const char *getVariable(const char*);
    void clearVariable(const char*);
    stringlist *listVariables();

    void update();
    char *expand(const char*, bool);

    CDvarTab *createBackup() const;
    void revertToBackup(const CDvarTab*);
    void destroyTab(CDvarTab*);

    // If a handler has been registered for a variable, it will be
    // called before the variable is actulally changed, with the
    // set-expanded value string (the actual new value when setting or
    // the present value when unsetting).
    //
    // The handler can call register_postfunc to register another
    // handler that will be called *after* the change is actually
    // made.  The postfunc is used once only.
    //
    CDvarProc registerInternal(const char*, CDvarProc);

    // Push/restore internal variables.  Call push_internal for each
    // variable to reset the action, and link the returns.  Call
    // restore_internal on the list to revert.
    //
    sVbak *pushInternal(sVbak*, const char*, CDvarProc);
    void restoreInternal(sVbak*);
    stringlist *listInternal();

private:
    static inline bool vcomp(const var_t*, const var_t*);

    table_t<var_t> *v_tab;          // the hash table for variables
    table_t<var_t> *v_tab_bak;      // backup table for resetting
    table_t<intvar_t<CDvarProc> > *v_regvar_tab;
                                    // table of registered variable names

    CDvarSetHookProc v_global_set_hook;
    CDvarGetHookProc v_global_get_hook;
    CDvarPostProc v_post_func;

    static cCDvdb *instancePtr;
};

#endif


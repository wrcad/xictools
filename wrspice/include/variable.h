
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: variable.h,v 2.62 2015/12/12 04:39:33 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef VARIABLE_H
#define VARIABLE_H


// references
struct variable;
struct wordlist;
struct sHtab;

enum VTYPenum
{
    VTYP_NONE,
    VTYP_BOOL,
    VTYP_NUM,
    VTYP_REAL,
    VTYP_STRING,
    VTYP_LIST
};

union va_t
{
    bool v_bool;
    int v_num;
    double v_real;
    char *v_string;
    variable *v_list;
};

// Variables that are accessible to the parser via $varname
// expansions.  If the type is VTYP_LIST the value is a pointer to a
// list of the elements.
//
struct variable
{
    // vardb.cc
    variable(const char* = 0);
    ~variable();

    void set_boolean(bool);
    void set_integer(int);
    void set_real(double);
    void set_string(const char*);
    void set_list(variable*);
    void set_reference(const char*);

    const char *typeString(int) const;
    variable *copy() const;
    wordlist *varwl() const;
    wordlist *var2wl(int, int) const;

    void clear()
        {
            if (va_type == VTYP_STRING)
                delete [] va.v_string;
            else if (va_type == VTYP_LIST)
                va.v_list->free();
            va_type = VTYP_NONE;
            va.v_real = 0.0;
        }

    void free()
        {
            variable *v = this;
            while (v) {
                variable *vx = v;
                v = v->va_next;
                delete vx;
            }
        }

    // The v_list is not copied in set_list, but is freed in the
    // destructor.  Call this to prevent destruction.
    //
    void zero_list()
        { 
            if (va_type == VTYP_LIST)
                va.v_list = 0;
        }

    VTYPenum type()             const { return (va_type); }
    const char *name()          const { return (va_name); }
    variable *next()            { return (va_next); }
    void set_next(variable *n)  { va_next = n; }

    bool boolean() const
        { return (va_type == VTYP_BOOL ? va.v_bool : false); }
    int integer() const
        { return (va_type == VTYP_NUM ? va.v_num : 0); }
    double real() const
        { return (va_type == VTYP_REAL ? va.v_real : 0.0); }
    const char *string() const
        { return (va_type == VTYP_STRING ? va.v_string : 0); }
    variable *list() const
        { return (va_type == VTYP_LIST ? va.v_list : 0); }

    const char *reference()     const { return (va_reference); }

    // In some cases, there is a need for a void* to the data union.
    const va_t *dataptr()       const { return (&va); }

private:
    VTYPenum va_type;
    const char *va_name;        // variable's name
    const char *va_reference;   // string describing the variable
    variable *va_next;
    va_t va;
};


struct sVarDb
{
    sVarDb() { variables = 0; }
    // no destructor, never freed

    sHtab *table() { return (variables); }

    unsigned int allocated();
    variable *get(const char*);
    void add(const char*, variable*);
    bool remove(const char*);
    wordlist *wl();

private:
    sHtab *variables;
};

#endif // VARIABLE_H


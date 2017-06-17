
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
 $Id: si_variable.h,v 5.2 2015/10/01 22:44:09 stevew Exp $
 *========================================================================*/

#ifndef SI_VARIABLE_H
#define SI_VARIABLE_H

#include "si_if_variable.h"


//
// Variable definitions, for internal use.
//

// Hold the layer desc, plus cell and symbol table names which will
// originate geometry.  These revert to the current cell and symbol
// table if null.  This holds the parse result of the
// layername[.stname][.cellname] form in layer expressions. 
// Variables with TYP_LDORIG hold one of these.

struct LDorig
{
    LDorig(const char*);

    ~LDorig()
        {
            delete [] ldo_origname;
        }

    CDl *ldesc()            const { return (ldo_ldesc); }
    CDcellName cellname()   const { return (ldo_cellname); }
    CDcellName stab_name()  const { return (ldo_stname); }
    const char *origname()  const { return (ldo_origname); }

private:
    CDl         *ldo_ldesc;
    CDcellName  ldo_cellname;
    CDcellName  ldo_stname;
    char        *ldo_origname;
};

// Types for parse nodes (required by variable definitions).
enum
{
    PT_BOGUS    = 0,
    PT_FUNCTION = 1,
    PT_CONSTANT = 2,
    PT_VAR      = 3,
    PT_BINOP    = 4,
    PT_UNOP     = 5
};
typedef unsigned char PNodeType;

struct ParseNode;
struct Variable;

// Array data for variables.
//
struct siAryData : public AryData
{
    bool init(int*);
    bool dimensions(ParseNode*, int*, int*, int*, int);
    bool check(ParseNode*);
    bool resolve(Variable*, ParseNode*);
    bool assign(Variable*, ParseNode*, ParseNode*, void*);
    bool expand(int*);
    bool resize(int);
    bool reset();
    bool mkpointer(Variable*, int);
};

#define ADATA(a) ((siAryData*)a)

// Note about memory allocation:
// A literal string has the content of the string in the name field,
// and a pointer reference is propagated on assignment in the string
// field of the content union.  If an evaluation yields a new string,
// the 'original' flag should be set in the returned node.
//
// Arrays are similar, except that the 'original' flag is set in the
// defining variable.  If a pointer to an array is defined with an
// offset, as in
// a[100]; let b = a + 1
// then the variable b has its own AryData struct, with the values
// field pointing to the a.AryData values + 1, and the refptr set
// to a.AryData.  In this case, the size of the array can not be
// redefined through b, and when b is freed, its AryData is freed but
// not the values.

struct rvals;

struct siVariable : public Variable
{
    siVariable() { };
    siVariable(char*, int*);

    // Type-specific helper functions.

    // Function for delete operator, safe within scripts.
    void safe_delete()
        {
            if (IN_TABLE(type) && safe_del_tab[type])
                (*safe_del_tab[type])(this);
        }

    // Called on arguments vectors after use.  Clean up.
    void gc_argv(Variable *res, PNodeType ntype)
        {
            if (IN_TABLE(type) && gc_argv_tab[type])
                (*gc_argv_tab[type])(this, res, ntype);
        }

    // Called on a function or operator return that is not used.  Clean up.
    void gc_result()
        {
            if (IN_TABLE(type) && gc_result_tab[type])
                (*gc_result_tab[type])(this);
        }

    // Take care of assignment during parse tree evaluation.
    bool assign(ParseNode *p, Variable *res, void *datap)
        {
            if (IN_TABLE(type)) {
                if (assign_tab[type])
                    return ((*assign_tab[type])(this, p, res, datap));
                return (OK);
            }
            return (BAD);
        }

    // The variable has just had the content set (in assignment), and
    // was previously TYP_NOTYPE.
    void assign_fix(Variable *r)
        {
            if (IN_TABLE(type) && assign_fix_tab[type])
                (*assign_fix_tab[type])(this, r);
        }

    // Core of evaluation function for PT_VAR.
    bool get_var(ParseNode *p, Variable *res)
        {
            if (IN_TABLE(type)) {
                if (get_var_tab[type])
                    return ((*get_var_tab[type])(this, p, res));
                return (OK);
            }
            return (BAD);
        }

    // Look-ahead check for binary operators.  If this returns true,
    // the operation can be skipped.
    bool bop_look_ahead(ParseNode *p, Variable *res)
        {
            if (IN_TABLE(type) && bop_look_ahead_tab[type])
                return ((*bop_look_ahead_tab[type])(this, p, res));
            return (false);
        }

    // Print function for debugger.
    void print_var(rvals *rv, char *buf)
        {
            if (IN_TABLE(type)) {
                if (print_var_tab[type])
                    (*print_var_tab[type])(this, rv, buf);
                else
                    strcpy(buf, "unprintable variable type");
            }
            else
                strcpy(buf, "internal: invalid type");
        }

    // Set variable function for debugger.
    char *set_var(rvals *rv, const char *val)
        {
            if (IN_TABLE(type)) {
                if (set_var_tab[type])
                    return ((*set_var_tab[type])(this, rv, val));
                return (lstring::copy("variable not settable"));
            }
            return (lstring::copy("invalid type"));
        }

    // Response function for server mode.
    int respond(int g, bool longform)
        {
            if (IN_TABLE(type)) {
                if (respond_tab[type])
                    return ((*respond_tab[type])(this, g, longform));
                return (-1);
            }
            return (-1);
        }

    // A set of dispatch tables for the functions defined above.

    static void(*safe_del_tab[NUM_TYPES])(Variable*);
    static void(*gc_argv_tab[NUM_TYPES])(Variable*, Variable*, PNodeType);
    static void(*gc_result_tab[NUM_TYPES])(Variable*);
    static bool(*assign_tab[NUM_TYPES])(siVariable*, ParseNode*, Variable*,
        void*);
    static void(*assign_fix_tab[NUM_TYPES])(Variable*, Variable*);
    static bool(*get_var_tab[NUM_TYPES])(Variable*, ParseNode*, Variable*);
    static bool(*bop_look_ahead_tab[NUM_TYPES])(Variable*, ParseNode*,
        Variable*);
    static void(*print_var_tab[NUM_TYPES])(Variable*, rvals*, char*);
    static char*(*set_var_tab[NUM_TYPES])(Variable*, rvals*, const char*);
    static int(*respond_tab[NUM_TYPES])(Variable*, int, bool);
};

#endif



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
 $Id: si_if_variable.h,v 5.7 2017/04/13 17:06:24 stevew Exp $
 *========================================================================*/

#ifndef SI_IF_VARIABLE_H
#define SI_IF_VARIABLE_H

#include <string.h>


//
// Exportable variable definitions.  This contains base objects for
// data representation.  The application subclasses these, adding
// specialized functions for internal use.
//

// Ubiquitous return values.
#define OK false
#define BAD true

// Flags for variables.
//
#define VF_ORIGINAL  0x1
#define VF_STATIC    0x2
#define VF_GLOBAL    0x4
#define VF_NAMED     0x8

// VF_ORIGINAL    If set, free string or array when variable is deleted.
// VF_STATIC      Variable is static in function.
// VF_GLOBAL      Variable has global scope.
// VF_NAMED       Variable is pointer to a named variable.

// Variable types.  This must match dispatch table for Variable class.
enum
{
    TYP_ENDARG = -1,
    TYP_NOTYPE =  0,
    TYP_STRING =  1,
    TYP_SCALAR =  2,
    TYP_ARRAY  =  3,
    TYP_CMPLX  =  4,
    TYP_ZLIST  =  5,
    TYP_LEXPR  =  6,
    TYP_HANDLE =  7,
    TYP_LDORIG =  8
};
typedef short VarType;

// TYP_NOTYPE     uninitialized variable
// TYP_STRING     string
// TYP_SCALAR     scalar number
// TYP_ARRAY      array of numbers
// TYP_CMPLX      complex number
// TYP_ZLIST      trapezoid list
// TYP_LEXPR      layer expression tree
// TYP_HANDLE     handle to something
// TYP_LDORIG     layer desc an origin, for layer expressions only
// This is the number of TYP_* values.
#define NUM_TYPES 9
// Validity test.
#define IN_TABLE(x) ((x) >= TYP_NOTYPE && (x) < NUM_TYPES)

#define MAXDIMS 3

// Data array description.
//
struct AryData
{
    AryData()
        {
            ad_values = 0;
            memset(ad_dims, 0, sizeof(ad_dims));
            ad_refcnt = 0;
            ad_refptr = 0;
        }

    ~AryData()
        {
            if (!ad_refptr)
                free(ad_values);
        }

    // Use this for allocation, ad_values always allocated with C
    // allocators.
    //
    void allocate(int sz)
        {
            free(ad_values);
            if (sz > 0)
                ad_values = (double*)calloc(sz, sizeof(double));
            else
                ad_values = 0;
        }

    int length() const {
        int size = ad_dims[0];
        for (int i = 1; i < MAXDIMS; i++) {
            if (ad_dims[i])
                size *= ad_dims[i];
            else
                break;
        }
        return (size);
    }

    double *values()        const { return (ad_values); }
    int *dims()             { return (ad_dims); }
    AryData *refptr()       const { return (ad_refptr); }

    int  refcnt()           const { return (ad_refcnt); }
    void incref()           { ad_refcnt++; }
    void decref()           { if (ad_refcnt) ad_refcnt--; }

protected:
    double *ad_values;      // Data.
    int ad_dims[MAXDIMS];   // Dimensions of array.  Arrays can have
                            // 1,2, or 3 dimensions.
    int ad_refcnt;          // Number of pointers to this data.
    AryData *ad_refptr;     // Set if this variable is a pointer.

    // If either of the two "ref" fields is non-zero, don't change the
    // ad_values array.  Otherwise, the ad_values array and/or the
    // dimensions can be reset in the called function.
};

struct Zlist;
struct sLspec;
struct LDorig;


// Conversion of real to bool.
//
inline bool to_boolean(double d)
{
    if (d < 0)
        d = -d;
    return (d >= 1e-37);
}


// Complex number.  No constructor as this is used in a union in
// Variable.
//
struct Cmplx
{
    double mag()
        {
            return (sqrt(real*real + imag*imag));
        }

    // Compute the square root, in place.
    //
    void csqrt()
        {
            if (real == 0.0) {
                if (imag == 0.0)
                    return;
                if (imag > 0.0) {
                    real = sqrt(0.5*imag);
                    imag = real;
                }
                else {
                    imag = -sqrt(-0.5*imag);
                    real = -imag;
                }
            }
            else if (real > 0.0) {
                if (imag == 0.0)
                    real = sqrt(real);
                else {
                    double d = mag();
                    real = sqrt(0.5*(d + real));
                    imag /= 2.0*real;
                }
            }
            else {
                if (imag == 0.0) {
                    imag = sqrt(-real);
                    real = 0.0;
                }
                else {
                    double d = mag();
                    double t = imag;
                    imag = sqrt(0.5*(d - real));
                    if (t < 0.0)
                        imag = -imag;
                    real = t/(2.0*imag);
                }
            }
        }

    double real;
    double imag;
};

// Description of a variable, passed to the evaluation functions.
//
struct Variable
{
    Variable()
        {
            name = 0;
            next = 0;
            content.string = 0;
            type = TYP_NOTYPE;
            flags = 0;
        }

    // Free a list of variables.  This frees the content, unlike the
    // destructor.
    //
    static void destroy(Variable *v)
        {
            while (v) {
                Variable *vx = v;
                v = v->next;
                delete [] vx->name;
                vx->clear();
                delete vx;
            }
        }

    // Type-specific helper functions.

    // Free content, call before destructor.  The destructor does NOT
    // free content.
    void clear()
        {
            if (IN_TABLE(type) && clear_tab[type])
                (*clear_tab[type])(this);
        }

    bool istrue()
        {
            if (type == TYP_SCALAR)
                return (to_boolean(content.value));
            if (type == TYP_HANDLE)
                return (istrue__handle());
            if (type == TYP_ARRAY) {
                if (content.a && content.a->values() &&
                        to_boolean(content.a->values()[0]))
                    return (true);
                return (false);
            }
            if (type == TYP_CMPLX) {
                return (to_boolean(content.cx.real) ||
                    to_boolean(content.cx.imag));
            }
            // applies to any of the pointers in union
            return (content.string != 0);
        }

    void set(Variable *v)
        {
            name = v->name;
            type = v->type;
            flags = v->flags;
            content = v->content;
        }

    void set_result(Variable *v)
        {
            name = v->name;
            type = v->type;
            flags = 0;
            content = v->content;
            next = 0;
        }

    bool istrue__handle();
    void incr__handle(Variable*);
    void cat__handle(Variable*, Variable*);

    char *name;             // The Variable's name, if any.
    Variable *next;         // Hook for listing.
    union {
        double value;
        Cmplx cx;
        AryData *a;
        char *string;
        Zlist *zlist;
        sLspec *lspec;
        LDorig *ldorig;
    } content;
    VarType type;           // Type of data, e.g. TYP_SCALAR.
    unsigned short flags;   // Set VF_ORIGINAL if data malloc'ed in
                            // called function, otherwise for internal
                            // use.

    // A dispatch table for the clear function defined above.
    static void(*clear_tab[NUM_TYPES])(Variable*);
};


// Derived from Cmplx, provides a constructor for convenience.
//
struct siCmplx : public Cmplx
{
    siCmplx(double r = 0.0, double i = 0.0)
        {
            real = r;
            imag = i;
        }

    siCmplx(const Variable &v)
        {
            if (v.type == TYP_CMPLX) {
                real = v.content.cx.real;
                imag = v.content.cx.imag;
            }
            else if (v.type == TYP_SCALAR) {
                real = v.content.value;
                imag = 0.0;
            }
            else if (v.type == TYP_ARRAY && v.content.a->values()) {
                int l = v.content.a->dims()[0];
                real = l > 0 ? v.content.a->values()[0] : 0.0;
                imag = l > 1 ? v.content.a->values()[1] : 0.0;
            }
            else {
                real = 0.0;
                imag = 0.0;
            }
        }
};

#endif



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

#ifndef SI_ARGS_H
#define SI_ARGS_H

//
// Some inline utilities for argument checking.  These are used in the
// script interface functions.
//

#define ARG_CHK(x) { if (!(x)) return (BAD); }

#ifndef NO_EXTERNAL
// These are presently not supported in the plug-in kit.

// Exported from funcs_lexpr.cc
// The bool* return indicates whether the payload return is "const"
// or not, true indicates a copy.
extern XIrt arg_zlist(const Variable*, int, Zlist**, bool*, void*);
extern bool arg_lexpr(const Variable*, int, sLspec**, bool*);

// Exported from funcs_misc3.cc
extern bool arg_layer(Variable*, int, CDl**, bool = false, bool = false);

#endif

// Record the argument index of the bad argument in the failure
// message.
//
extern void arg_record_error(int);

//
// In the processing functions below, the indx is a 0-based argument
// count from left to right.  It is an index into the arguments array
// passed in the first argument.  The third and additional arguments
// represent the data item as a C++ data type pointer.
//

// Real number.
//
inline bool
arg_real(const Variable *args, int indx, double *val)
{
    if (args[indx].type == TYP_SCALAR) {
        *val = args[indx].content.value;
        return (true);
    }
    if (args[indx].type == TYP_CMPLX) {
        *val = args[indx].content.cx.real;
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// Complex number.
//
inline bool
arg_cmplx(const Variable *args, int indx, Cmplx *cx)
{
    if (args[indx].type == TYP_CMPLX) {
        *cx = args[indx].content.cx;
        return (true);
    }
    if (args[indx].type == TYP_SCALAR) {
        cx->real = args[indx].content.value;
        cx->imag = 0.0;
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// Integer.
//
inline bool
arg_int(const Variable *args, int indx, int *val)
{
    if (args[indx].type == TYP_SCALAR) {
        *val = (int)args[indx].content.value;
        return (true);
    }
    if (args[indx].type == TYP_CMPLX) {
        *val = (int)args[indx].content.cx.real;
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// Unsigned integer.
//
inline bool
arg_unsigned(const Variable *args, int indx, unsigned int *val)
{
    if (args[indx].type == TYP_SCALAR) {
        *val = (unsigned int)args[indx].content.value;
        return (true);
    }
    if (args[indx].type == TYP_CMPLX) {
        *val = (unsigned int)args[indx].content.cx.real;
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// Boolean.
//
inline bool
arg_boolean(const Variable *args, int indx, bool *val)
{
    if (args[indx].type == TYP_SCALAR) {
        *val = to_boolean(args[indx].content.value);
        return (true);
    }
    if (args[indx].type == TYP_CMPLX) {
        *val = to_boolean(args[indx].content.cx.real) ||
            to_boolean(args[indx].content.cx.imag);
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// Integer coordinate.
//
inline bool
arg_coord(const Variable *args, int indx, int *val, DisplayMode mode)
{
    if (args[indx].type == TYP_SCALAR) {
        if (mode == Physical)
            *val = INTERNAL_UNITS(args[indx].content.value);
        else
            *val = ELEC_INTERNAL_UNITS(args[indx].content.value);
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// Handle, accept scalar 0 as empty handle.
//
inline bool
arg_handle(const Variable *args, int indx, int *val)
{
    if (args[indx].type == TYP_HANDLE) {
        *val = (int)args[indx].content.value;
        return (true);
    }
    if (args[indx].type == TYP_SCALAR &&
            !to_boolean(args[indx].content.value)) {
        *val = 0;
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// String, accepts scalar 0 as null string.
//
inline bool
arg_string(const Variable *args, int indx, const char **val)
{
    if (args[indx].type == TYP_STRING || args[indx].type == TYP_NOTYPE) {
        *val = args[indx].content.string;
        return (true);
    }
    if (args[indx].type == TYP_SCALAR &&
            !to_boolean(args[indx].content.value)) {
        *val = 0;
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// Hierarchy depth argument handler, accepts integers and string "all".
//
inline bool
arg_depth(const Variable *args, int indx, int *val)
{
    if (args[indx].type == TYP_SCALAR) {
        *val = (int)args[indx].content.value;
        if (*val < 0)
            *val = CDMAXCALLDEPTH;
        return (true);
    }
    if (args[indx].type == TYP_STRING || args[indx].type == TYP_NOTYPE) {
        if (args[indx].content.string && *args[indx].content.string == 'a') {
            *val = CDMAXCALLDEPTH;
            return (true);
        }
    }
    arg_record_error(indx);
    return (false);
}


// Array, size must be minsize or larger.
//
inline bool
arg_array(const Variable *args, int indx, double **vals, int minsize)
{
    if (minsize < 1)
        return (false);
    if (args[indx].type == TYP_ARRAY && args[indx].content.a->values() &&
            args[indx].content.a->length() >= minsize) {
        *vals = args[indx].content.a->values();
        return (true);
    }
    arg_record_error(indx);
    return (false);
}


// Array, as above, or scalar 0.
//
inline bool
arg_array_if(const Variable *args, int indx, double **vals, int minsize)
{
    if (minsize < 1)
        return (false);
    if (args[indx].type == TYP_ARRAY && args[indx].content.a->values() &&
            args[indx].content.a->length() >= minsize) {
        *vals = args[indx].content.a->values();
        return (true);
    }
    if (args[indx].type == TYP_SCALAR &&
            !to_boolean(args[indx].content.value)) {
        *vals = 0;
        return (true);
    }
    arg_record_error(indx);
    return (false);
}

#endif


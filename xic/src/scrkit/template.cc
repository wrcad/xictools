
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

//
// Example script function plug-in for Xic.
//

// Always include scrkit.h, this provides the interface definitions
// for Xic.  You will need to add other headers as appropriate for
// your code.
//
#include "scrkit.h"
#include <stdio.h>


// Put the script functions in an anonymous namespace, we don't need
// (or want) the function names exported.
//
namespace {

    // Below is a function which will be callable from scripts.  All
    // such functions have this prototype.  You can define as many
    // such functions as are needed.  The function names are
    // arbitrary, but generally very similar to the name by which the
    // function is called in scripts.

    // (int) my_Func(real, integer, boolean, string, array)
    //
    bool my_Func(Variable *res, Variable *args, void*)
    {
        // Unless an argument requires special handling, the macros
        // and utilities defined in si_args.h are used to handle the
        // arguments.  The ARG_CHK will return BAD on a bad argument. 
        // A return of BAD is deemed unrecoverable and will halt the
        // scripts.

        // Note that the second argument is the argument placement
        // (0-based) in the script function call.

        double val;
        ARG_CHK(arg_real(args, 0, &val))

        int ival;
        ARG_CHK(arg_int(args, 1, &ival))

        bool bval;
        ARG_CHK(arg_boolean(args, 2, &bval))

        const char *sval;
        ARG_CHK(arg_string(args, 3, &sval))

        double *avals;
        ARG_CHK(arg_array(args, 4, &avals, 1))
        int asize = args[4].content.a->length();

        // The function body is up to the user.  Here, we simply print
        // some info about the arguments.  Returns can be implemented
        // by setting the res Variable struct, or/and changing the
        // Variables passed as arguments.

        printf("arg 1 is real, value is %g\n", val);
        printf("arg 2 is integer, value is %d\n", ival);
        printf("arg 3 is boolean, value is %s\n", bval ? "true" : "false");
        printf("arg 4 is string, value is %s\n", sval);
        printf("arg 5 is an array, size is %d value[0] is %g\n", asize,
            avals[0]);

        // Return a scalar 1.0.
        res->type = TYP_SCALAR;
        res->content.value = 1.0;

        /* Some other return options:
        // Return a string.
        res->type = TYP_STRING;
        res->content.string = strdup("this is a string");
        res->flags |= VF_ORIGINAL;  // Will free string when finished.

        // Return a const string (never freed).
        res->type = TYP_STRING;
        res->content.string = (char*)"this is a string";

        // Return an array of size 10 containing values 0-9
        res->type = TYP_ARRAY;
        res->content.a = new AryData;
        res->content.a->dims[0] = 10;
        res->content.a->data = new double[10];
        for (int i = 0; i < 10; i++)
            res->content.a->values[i] = i;
        res->flags |= VF_ORIGINAL;  // Will free array when finished.
        */

        return (OK);
    }


    // Here's another example.  This functions will take an arbitrary
    // number of numeric arguments, and return an array object
    // containing the values.  If not passed any arguments, it will
    // return a scalar 0.

    // (array) my_NewArray(value, ...)
    //
    bool my_NewArray(Variable *res, Variable *args, void*)
    {
        // Count the args.  If we find one that is not numeric, return
        // BAD.  The argument list will end with a TYP_ENDARG phony
        // argument.  We'll also allow for this to be missing, and all
        // possible arguments will be taken.

        int i = 0;
        for ( ; i < MAXARGC; i++) {
            if (args[i].type == TYP_ENDARG)
                break;
            if (args[i].type != TYP_SCALAR)
                return (BAD);
        }

        if (i == 0) {
            // No arguments, return 0.0.
            res->type = TYP_SCALAR;
            res->content.value = 0;
        }
        else {
            // Create the array and load the values.
            double *ary = new double[i];
            i = 0;
            for ( ; i < MAXARGC; i++) {
                if (args[i].type == TYP_ENDARG)
                    break;
                ary[i] = args[i].content.value;
            }

            // Below are steps to create the array object returned. 
            // Since we created the array here, set the VF_ORIGINAL
            // flag so that the array will be freed when we're done.

            res->type = TYP_ARRAY;
            res->content.a = new AryData;
            res->content.a->dims[0] = i;
            res->content.a->values = ary;
            res->flags |= VF_ORIGINAL;
        }
        return (OK);
    }
}


extern "C" {

    // Every plug-in requires an init function with this prototype.  The
    // function must be named "init".  This function MUST be defined in an
    // extern "C" block.
    //
    void init(const char *args)
    {
        // The passed string is the tail of the command string given to
        // the !ldshared command when loading the library.  For example,
        // if the command was "!ldshared template.so here is a string",
        // the args string passed here will be "here is a string".  The
        // user can do what they want with this, here we ignore it.

        (void)args;

        // The user can add code here to do any needed initialization. 
        // This function is executed once only, when the plug-in is
        // loaded.

        // This registerScriptFunc function must be called for every
        // script function defined in the plug in.  The first argument is
        // the function name, as given in the anonymous namespace above. 
        // The second argument is a string giving the name of the function
        // as known in scripts.  Note that it is possible to overwrite
        // internally-defined script functions, which is not usually a
        // good idea.  It is recommended that the user adopt some naming
        // standard which would keep their functions recognizable and not
        // collide with built-in functions.  For example, start all
        // functions with your initials in lower case.  It is usual, but
        // not required, that the function name and name string be the
        // same.
        //
        // The third argument is the number of arguments expected.  Xic
        // will check calls and abort the script if the argument count is
        // wrong.  This can also be the enum value VARARGS, which will
        // allow a variable number of arguments (up to 40).  For such
        // functions, arguments must be handled until a type TYP_ENDARG is
        // seen.

        registerScriptFunc(my_Func, "my_Func", 5);
        registerScriptFunc(my_NewArray, "my_NewArray", VARARGS);

    }

    // Every plug-in should have an uninit function with this
    // prototype, defined in an extern "C" block.  It should call the
    // unRegisterScriptFunc with the name of each function defined in
    // the plug-in, the same name string that was passed to
    // registerScriptFunc in init.
    //
    void uninit()
    {
        unRegisterScriptFunc("my_Func");
        unRegisterScriptFunc("my_NewArray");
    }

} // end of extern 'C'


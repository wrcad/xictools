
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2018 Whiteley Research Inc., all rights reserved.       *
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef FPE_CHECK_H
#define FPE_CHECK_H

// Common code for testing floating point exceptions, must be included
// after config.h and fenv.h (if defined).


namespace {
    // Floating point exception checking.  Exceptions can be enabled
    // in wrspice.cc for debugging.
    //
    inline int check_fpe(bool noerrret)
    {
        int err = OK;
#ifdef HAVE_FENV_H
        if (!noerrret && Sp.GetFPEmode() == FPEcheck) {
            if (fetestexcept(FE_DIVBYZERO))
                err = E_MATHDBZ;
            else if (fetestexcept(FE_OVERFLOW))
                err = E_MATHOVF;
            // Ignore underflow, these really shouldn't be a problem.
            // else if (fetestexcept(FE_UNDERFLOW))
            //     err = E_MATHUNF;
            else if (fetestexcept(FE_INVALID))
                err = E_MATHINV;
        }
        feclearexcept(FE_ALL_EXCEPT);
#else
        (void)noerrret;
#endif
        return (err);
    }
}

#endif


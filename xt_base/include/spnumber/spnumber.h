
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef SPNUMBER_H
#define SPNUMBER_H

#include <string.h>


//
// WRspice number formatting, printing, parsing.
//

struct sUnits;

#define SPN_BUFSIZ 512
#define SPN_CHUNK 40

struct sSPnumber
{
    sSPnumber()
        {
            np_string[0] = 0;
            np_offs = 0;
        }

    const char *printnum(double, const sUnits*, bool = false, int = 0);
    const char *printnum(double, const char* = 0, bool = false, int = 0);
    const char *print_exp(double, int);

    const char *fixxp2(char*);
    static const char *suffix(int);

    double *parse(const char**, bool, bool = false, sUnits** = 0);

private:
    // Obtain some buffer space.
    char *newbuf()
    {
        int lim = SPN_BUFSIZ - SPN_CHUNK;
        // Skip past last number printed.
        while (np_offs < lim && np_string[np_offs])
            np_offs++;
        if (np_offs >= lim) // Not enough space left.
            np_offs = 0;
        return (np_offs ? np_string + ++np_offs : np_string);
    }

    static bool get_unitstr(const char**, char*);

    double np_num;
    char np_string[SPN_BUFSIZ];
    int np_offs;

    static double np_powers[];
};

extern sSPnumber SPnum;

#endif  // SP_NUMBER_H


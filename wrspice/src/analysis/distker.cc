
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jaijeet S Roychowdhury
         1993 Stephen R. Whiteley
****************************************************************************/

#include <math.h>
#include "distdefs.h"
#include "errors.h"


int
DkerProc(int type, double *rPtr, double *iPtr, int size, sDISTOAN *job)
{
    (void)job;
    int i;

    switch(type) {

    case D_F1:

        for (i = 1; i <= size; i++) {
            iPtr[i] *= 2.0; /* convert to sinusoid amplitude */
            rPtr[i] *= 2.0;
        }
        break;

    case D_F2:

        for (i = 1; i <= size; i++) {
            rPtr[i] *= 2.0;
            iPtr[i] *= 2.0;
        }
        break;

    case D_TWOF1:

        for (i = 1; i <= size; i++) {
            iPtr[i] *= 2.0;
            rPtr[i] *= 2.0;
        }
        break;

    case D_THRF1:

        for (i = 1; i <= size; i++) {
            iPtr[i] *= 2.0;
            rPtr[i] *= 2.0;
        }
        break;

    case D_F1PF2:

        for (i = 1; i <= size; i++) {
            iPtr[i] *= 4.0;
            rPtr[i] *= 4.0;
        }
        break;

    case D_F1MF2:

        for (i = 1; i <= size; i++) {
            iPtr[i] *= 4.0;
            rPtr[i] *= 4.0;
        }
        break;

    case D_2F1MF2:

        for (i = 1; i <= size; i++) {
            iPtr[i] *= 6.0;
            rPtr[i] *= 6.0;
        }
        break;

    default:

        return (E_BADPARM);

    }
    return (OK);
}

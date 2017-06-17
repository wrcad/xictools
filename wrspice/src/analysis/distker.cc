
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
 $Id: distker.cc,v 2.20 2000/11/19 00:37:09 stevew Exp $
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

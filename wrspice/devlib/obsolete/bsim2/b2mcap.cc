
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


// routine to calculate equivalent conductance and total terminal 
// charges
//
void
B2dev::mosCap(sCKT *ckt, double vgd, double vgs, double vgb, double args[8],
    double cbgb, double cbdb, double cbsb, double cdgb, double cddb,
    double cdsb, double *gcggbPointer, double *gcgdbPointer,
    double *gcgsbPointer, double *gcbgbPointer, double *gcbdbPointer,
    double *gcbsbPointer, double *gcdgbPointer, double *gcddbPointer,
    double *gcdsbPointer, double *gcsgbPointer, double *gcsdbPointer,
    double *gcssbPointer, double *qGatePointer, double *qBulkPointer,
    double *qDrainPointer, double *qSourcePointer)
{
    double qgd;
    double qgs;
    double qgb;
    double ag0;

    ag0 = ckt->CKTag[0];
    // compute equivalent conductance
    *gcdgbPointer = (cdgb - args[0]) * ag0;
    *gcddbPointer = (cddb + args[3] + args[0]) * ag0;
    *gcdsbPointer = cdsb * ag0;
    *gcsgbPointer = -(args[5] + cbgb + cdgb + args[1]) * ag0;
    *gcsdbPointer = -(args[6] + cbdb + cddb ) * ag0;
    *gcssbPointer = (args[4] + args[1] - 
        (args[7] + cbsb + cdsb )) * ag0;
    *gcggbPointer = (args[5] + args[0] +
        args[1] + args[2] ) * ag0;
    *gcgdbPointer = (args[6] - args[0]) * ag0;
    *gcgsbPointer = (args[7] - args[1]) * ag0;
    *gcbgbPointer = (cbgb - args[2]) * ag0;
    *gcbdbPointer = (cbdb - args[3]) * ag0;
    *gcbsbPointer = (cbsb - args[4]) * ag0;
 
    // compute total terminal charge
    qgd = args[0] * vgd;
    qgs = args[1] * vgs;
    qgb = args[2] * vgb;
    *qGatePointer = *qGatePointer + qgd + qgs + qgb;
    *qBulkPointer = *qBulkPointer - qgb;
    *qDrainPointer = *qDrainPointer - qgd;
    *qSourcePointer = -(*qGatePointer + *qBulkPointer + *qDrainPointer);
}




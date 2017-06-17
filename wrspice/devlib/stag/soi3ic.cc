
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
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: soi3ic.cc,v 2.10 2002/12/03 01:01:37 stevew Exp $
 *========================================================================*/

/**********
STAG version 2.6
Copyright 2000 owned by the United Kingdom Secretary of State for Defence
acting through the Defence Evaluation and Research Agency.
Developed by :     Jim Benson,
                   Department of Electronics and Computer Science,
                   University of Southampton,
                   United Kingdom.
With help from :   Nele D'Halleweyn, Bill Redman-White, and Craig Easson.

Based on STAG version 2.1
Developed by :     Mike Lee,
With help from :   Bernard Tenbroek, Bill Redman-White, Mike Uren, Chris Edwards
                   and John Bunyan.
Acknowledgements : Rupert Howes and Pete Mole.
**********/

#include "soi3defs.h"

#define SOI3nextModel      next()
#define SOI3nextInstance   next()
#define SOI3instances      inst()


int
SOI3dev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sSOI3model *model = static_cast<sSOI3model*>(genmod);
    sSOI3instance *here;
    /*
     * grab initial conditions out of rhs array.   User specified, so use
     * external nodes to get values
     */

    for( ; model ; model = model->SOI3nextModel) {
        for(here = model->SOI3instances; here ; here = here->SOI3nextInstance) {
            if(!here->SOI3icVBSGiven) {
                here->SOI3icVBS =
                        *(ckt->CKTrhs + here->SOI3bNode) -
                        *(ckt->CKTrhs + here->SOI3sNode);
            }
            if(!here->SOI3icVDSGiven) {
                here->SOI3icVDS =
                        *(ckt->CKTrhs + here->SOI3dNode) -
                        *(ckt->CKTrhs + here->SOI3sNode);
            }
            if(!here->SOI3icVGFSGiven) {
                here->SOI3icVGFS =
                        *(ckt->CKTrhs + here->SOI3gfNode) -
                        *(ckt->CKTrhs + here->SOI3sNode);
            }
            if(!here->SOI3icVGBSGiven) {
                here->SOI3icVGBS =
                        *(ckt->CKTrhs + here->SOI3gbNode) -
                        *(ckt->CKTrhs + here->SOI3sNode);
            }
        }
    }
    return(OK);
}


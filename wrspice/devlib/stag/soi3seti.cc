
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
 $Id: soi3seti.cc,v 2.13 2015/07/29 04:50:20 stevew Exp $
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

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


int
SOI3dev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sSOI3instance *here = static_cast<sSOI3instance*>(geninst);
    IFvalue *value = &data->v;

    switch(param) {
    case SOI3_L:
        here->SOI3l = value->rValue;
        here->SOI3lGiven = TRUE;
        break;
    case SOI3_W:
        here->SOI3w = value->rValue;
        here->SOI3wGiven = TRUE;
        break;
    case SOI3_NRD:
        here->SOI3drainSquares = value->rValue;
        here->SOI3drainSquaresGiven = TRUE;
        break;
   case SOI3_NRS:
        here->SOI3sourceSquares = value->rValue;
        here->SOI3sourceSquaresGiven = TRUE;
        break;
    case SOI3_OFF:
        here->SOI3off = value->iValue;
        break;
    case SOI3_IC_VDS:
        here->SOI3icVDS = value->rValue;
        here->SOI3icVDSGiven = TRUE;
        break;
    case SOI3_IC_VGFS:
        here->SOI3icVGFS = value->rValue;
        here->SOI3icVGFSGiven = TRUE;
        break;
    case SOI3_IC_VGBS:
        here->SOI3icVGBS = value->rValue;
        here->SOI3icVGBSGiven = TRUE;
        break;
    case SOI3_IC_VBS:
        here->SOI3icVBS = value->rValue;
        here->SOI3icVBSGiven = TRUE;
        break;
    case SOI3_TEMP:
        here->SOI3temp = value->rValue+CONSTCtoK;
        here->SOI3tempGiven = TRUE;
        break;
    case SOI3_RT:
        here->SOI3rt = value->rValue;
        here->SOI3rtGiven = TRUE;
        break;
    case SOI3_CT:
        here->SOI3ct = value->rValue;
        here->SOI3ctGiven = TRUE;
        break;
    case SOI3_RT1:
        here->SOI3rt1 = value->rValue;
        here->SOI3rt1Given = TRUE;
        break;
    case SOI3_CT1:
        here->SOI3ct1 = value->rValue;
        here->SOI3ct1Given = TRUE;
        break;
    case SOI3_RT2:
        here->SOI3rt2 = value->rValue;
        here->SOI3rt2Given = TRUE;
        break;
    case SOI3_CT2:
        here->SOI3ct2 = value->rValue;
        here->SOI3ct2Given = TRUE;
        break;
    case SOI3_RT3:
        here->SOI3rt3 = value->rValue;
        here->SOI3rt3Given = TRUE;
        break;
    case SOI3_CT3:
        here->SOI3ct3 = value->rValue;
        here->SOI3ct3Given = TRUE;
        break;
    case SOI3_RT4:
        here->SOI3rt4 = value->rValue;
        here->SOI3rt4Given = TRUE;
        break;
    case SOI3_CT4:
        here->SOI3ct4 = value->rValue;
        here->SOI3ct4Given = TRUE;
        break;
    case SOI3_IC:
        switch(value->v.numValue){
        case 4:
            here->SOI3icVBS = *(value->v.vec.rVec+3);
            here->SOI3icVBSGiven = TRUE;
            // fallthrough
        case 3:
            here->SOI3icVGBS = *(value->v.vec.rVec+2);
            here->SOI3icVGBSGiven = TRUE;
            // fallthrough
        case 2:
            here->SOI3icVGFS = *(value->v.vec.rVec+1);
            here->SOI3icVGFSGiven = TRUE;
            // fallthrough
        case 1:
            here->SOI3icVDS = *(value->v.vec.rVec);
            here->SOI3icVDSGiven = TRUE;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return(E_BADPARM);
        }
        break;
/*      case SOI3_L_SENS:
        if(value->iValue) {
            here->SOI3senParmNo = 1;
            here->SOI3sens_l = 1;
        }
        break;
    case SOI3_W_SENS:
        if(value->iValue) {
            here->SOI3senParmNo = 1;
            here->SOI3sens_w = 1;
        }
        break;                            */
    default:
        return(E_BADPARM);
    }
    return(OK);
}


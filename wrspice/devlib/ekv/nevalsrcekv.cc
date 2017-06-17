
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
 $Id: nevalsrcekv.cc,v 2.9 2002/11/10 19:47:12 stevew Exp $
 *========================================================================*/

/*
 * Author: 2000 Wladek Grabinski; EKV v2.6 Model Upgrade
 * Author: 1997 Eckhard Brass;    EKV v2.5 Model Implementation
 *     (C) 1990 Regents of the University of California. Spice3 Format
 */

/*
 * NevalSrcEKV (noise, lnNoise, ckt, type, node1, node2, param)
 *   This routine evaluates the noise due to different physical
 *   phenomena.  This includes the "shot" noise associated with dc
 *   currents in semiconductors and the "thermal" noise associated with
 *   resistance.  Although semiconductors also display "flicker" (1/f)
 *   noise, the lack of a unified model requires us to handle it on a
 *   "case by case" basis.  What we CAN provide, though, is the noise
 *   gain associated with the 1/f source.
 */


/*
#include "spice.h"
#include "cktdefs.h"
#include "const.h"
#include "noisedef.h"
#include "util.h"
#include "suffix.h"
#include "ekvdefs.h"
*/
#include "ekvdefs.h"
#include "noisdefs.h"

#define MAX SPMAX

void
NevalSrcEKV (double temp, sEKVmodel *model, sEKVinstance *here, double *noise,
    double *lnNoise, sCKT *ckt, int type, int node1, int node2, double param)
/*
void
NevalSrcEKV (temp,model, here, noise, lnNoise, ckt, type, node1, node2, param)

double temp;
register EKVmodel *model;
register EKVinstance *here;
double *noise;
double *lnNoise;
CKTcircuit *ckt;
int type;
int node1;
int node2;
double param;
*/

{
	double realVal;
	double imagVal;
	double gain;
	double alpha, sqrtalpha_1, gamma;

	realVal = *((ckt->CKTrhs) + node1) - *((ckt->CKTrhs) + node2);
	imagVal = *((ckt->CKTirhs) + node1) - *((ckt->CKTirhs) + node2);
	gain = (realVal*realVal) + (imagVal*imagVal);
	switch (type) {

	case SHOTNOISE:
		*noise = gain * 2 * CHARGE * FABS(param);
		/* param is the dc current in a semiconductor */
		*lnNoise = log( MAX(*noise,N_MINLOG) );
		break;

	case THERMNOISE:
		if (model->EKVnlevel==1) {
			*noise = gain * 4 * CONSTboltz * temp * param;
			/* param is the conductance of a resistor */
		}
		else {
			alpha=here->EKVir/here->EKVif;
			sqrtalpha_1 = sqrt(alpha) + 1;
			gamma = 0.5 * (1.0 + alpha);
			gamma += 2.0/3.0 * here->EKVif * (sqrtalpha_1 + alpha);
			gamma /= sqrtalpha_1 * (1.0 + here->EKVif);
			*noise = gain * 4 * CONSTboltz * temp * gamma * fabs(here->EKVgms);
		}
		*lnNoise = log( MAX(*noise,N_MINLOG) );
		break;

	case N_GAIN:
		*noise = gain;
		break;

	}
}
